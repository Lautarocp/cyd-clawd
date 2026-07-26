#!/usr/bin/env python3
"""CYD Clawd - Servidor HTTP de uso de Claude Code para ESP32 CYD.

Lee el token OAuth de ~/.claude/.credentials.json (igual que Claude Code CLI),
consulta la API de Anthropic cada 60s, y sirve los datos en GET /usage.

Uso:
    python3 server.py

Requisitos:
    pip install httpx
"""

import asyncio
import json
import re
import sys
import time
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path

try:
    import httpx
except ImportError:
    sys.exit("Falta httpx. Instalá con: pip install httpx")

# ── Config ────────────────────────────────────────────────────────────────
PORT          = 8765
POLL_INTERVAL = 60   # segundos entre consultas a la API
CRED_FILE     = Path.home() / ".claude" / ".credentials.json"

OAUTH_TOKEN_URL = "https://platform.claude.com/v1/oauth/token"
OAUTH_CLIENT_ID = "9d1c250a-e61b-44d9-88ed-5944d1962f5e"
# Refrescar el token si vence en menos de este tiempo (segundos)
TOKEN_REFRESH_MARGIN = 300

API_URL = "https://api.anthropic.com/v1/messages"
API_HEADERS = {
    "anthropic-version": "2023-06-01",
    "anthropic-beta":    "oauth-2025-04-20",
    "Content-Type":      "application/json",
    "User-Agent":        "claude-code/2.1.5",
}
API_BODY = {
    "model":     "claude-haiku-4-5-20251001",
    "max_tokens": 1,
    "messages":  [{"role": "user", "content": "x"}],
}

# ── Estado compartido (daemon → HTTP handler) ─────────────────────────────
_lock        = threading.Lock()
_cached_data = {"ok": False, "st": "starting"}


def log(msg: str) -> None:
    print(f"[{time.strftime('%H:%M:%S')}] {msg}", flush=True)


# ── Lectura y refresco del token OAuth ───────────────────────────────────

def _read_creds() -> dict | None:
    if not CRED_FILE.exists():
        log(f"No se encontró {CRED_FILE}")
        return None
    try:
        data = json.loads(CRED_FILE.read_text().strip())
    except (OSError, json.JSONDecodeError) as e:
        log(f"Error leyendo credenciales: {e}")
        return None
    # Estructura conocida: {"claudeAiOauth": {...}}
    if isinstance(data, dict):
        for v in [data] + list(data.values()):
            if isinstance(v, dict) and "accessToken" in v:
                return v
    return None


def _save_creds(creds: dict) -> None:
    try:
        data = json.loads(CRED_FILE.read_text().strip())
    except (OSError, json.JSONDecodeError):
        data = {}
    # Actualizar la clave que contiene el accessToken
    updated = False
    if isinstance(data, dict):
        if "accessToken" in data:
            data.update(creds)
            updated = True
        else:
            for k, v in data.items():
                if isinstance(v, dict) and "accessToken" in v:
                    v.update(creds)
                    updated = True
                    break
    if not updated:
        data = creds
    try:
        CRED_FILE.write_text(json.dumps(data))
    except OSError as e:
        log(f"No se pudo guardar credenciales: {e}")


async def _refresh_token(refresh_token: str) -> str | None:
    body = {
        "grant_type":    "refresh_token",
        "refresh_token": refresh_token,
        "client_id":     OAUTH_CLIENT_ID,
    }
    try:
        async with httpx.AsyncClient(timeout=20.0) as client:
            resp = await client.post(
                OAUTH_TOKEN_URL,
                json=body,
                headers={"Content-Type": "application/json"},
            )
    except httpx.HTTPError as e:
        log(f"Error de red al refrescar token: {e}")
        return None

    if resp.status_code != 200:
        log(f"Refresh token falló HTTP {resp.status_code}")
        return None

    try:
        result = resp.json()
    except Exception:
        log("Respuesta de refresh inválida")
        return None

    new_access  = result.get("access_token")
    new_refresh = result.get("refresh_token")
    expires_in  = result.get("expires_in", 3600)

    if not new_access:
        log("Refresh no devolvió access_token")
        return None

    updated = {"accessToken": new_access, "expiresAt": int(time.time() * 1000) + expires_in * 1000}
    if new_refresh:
        updated["refreshToken"] = new_refresh
    _save_creds(updated)
    log("Token refrescado correctamente")
    return new_access


async def get_valid_token() -> str | None:
    creds = _read_creds()
    if not creds:
        return None

    access_token  = creds.get("accessToken")
    refresh_token = creds.get("refreshToken")
    expires_at_ms = creds.get("expiresAt", 0)  # milisegundos

    # Refrescar proactivamente si el token vence pronto
    expires_in_s = (expires_at_ms / 1000) - time.time()
    if expires_in_s < TOKEN_REFRESH_MARGIN and refresh_token:
        log(f"Token vence en {int(expires_in_s)}s, refrescando...")
        new_token = await _refresh_token(refresh_token)
        if new_token:
            return new_token

    return access_token


# ── Consulta a la API ─────────────────────────────────────────────────────

async def poll_api() -> dict | None:
    token = await get_valid_token()
    if not token:
        return None

    headers = dict(API_HEADERS)
    headers["Authorization"] = f"Bearer {token}"

    try:
        async with httpx.AsyncClient(timeout=20.0) as client:
            resp = await client.post(API_URL, headers=headers, json=API_BODY)
    except httpx.HTTPError as e:
        log(f"Error de red: {e}")
        return None

    # Si el token expiró en el medio, intentar refrescar y reintentar una vez
    if resp.status_code == 401:
        log("HTTP 401 — intentando refrescar token...")
        creds = _read_creds()
        refresh_token = creds.get("refreshToken") if creds else None
        if refresh_token:
            new_token = await _refresh_token(refresh_token)
            if new_token:
                headers["Authorization"] = f"Bearer {new_token}"
                try:
                    async with httpx.AsyncClient(timeout=20.0) as client:
                        resp = await client.post(API_URL, headers=headers, json=API_BODY)
                except httpx.HTTPError as e:
                    log(f"Error de red en reintento: {e}")
                    return None

    if resp.status_code >= 400:
        log(f"API devolvió HTTP {resp.status_code}")
        return None

    now = time.time()

    def hdr(name: str, default: str = "0") -> str:
        return resp.headers.get(name, default)

    def pct(util: str) -> int:
        try:
            return int(round(float(util) * 100))
        except ValueError:
            return 0

    def reset_mins(ts_str: str) -> int:
        try:
            mins = (float(ts_str) - now) / 60.0
            return max(0, int(round(mins)))
        except ValueError:
            return 0

    # Cuenta Pro/Max: ventanas 5h y 7d
    if resp.headers.get("anthropic-ratelimit-unified-5h-utilization"):
        payload = {
            "s":    pct(hdr("anthropic-ratelimit-unified-5h-utilization")),
            "sr":   reset_mins(hdr("anthropic-ratelimit-unified-5h-reset")),
            "w":    pct(hdr("anthropic-ratelimit-unified-7d-utilization")),
            "wr":   reset_mins(hdr("anthropic-ratelimit-unified-7d-reset")),
            "st":   hdr("anthropic-ratelimit-unified-5h-status", "unknown"),
            "acct": "pro",
            "ok":   True,
        }
    else:
        # Cuenta Enterprise: gasto vs límite
        payload = {
            "s":    pct(hdr("anthropic-ratelimit-unified-overage-utilization")),
            "sr":   reset_mins(hdr("anthropic-ratelimit-unified-overage-reset")),
            "w":    0,
            "wr":   0,
            "st":   hdr("anthropic-ratelimit-unified-status", "unknown"),
            "acct": "ent",
            "ok":   True,
        }

    # Reloj: epoch local para que el CYD muestre la hora sin RTC
    payload["t"]  = int(time.time()) + time.localtime().tm_gmtoff
    payload["tf"] = 24   # o 12 según preferencia

    return payload


# ── Daemon de polling ─────────────────────────────────────────────────────

async def polling_loop() -> None:
    global _cached_data
    while True:
        log("Consultando API...")
        data = await poll_api()
        if data:
            with _lock:
                _cached_data = data
            log(f"OK → s={data['s']}% w={data['w']}% st={data['st']}")
        else:
            with _lock:
                _cached_data["ok"] = False
                _cached_data["t"]  = int(time.time()) + time.localtime().tm_gmtoff
                _cached_data["tf"] = 24
            log("Sin datos este ciclo")
        await asyncio.sleep(POLL_INTERVAL)


def run_async_loop() -> None:
    asyncio.run(polling_loop())


# ── HTTP Handler ──────────────────────────────────────────────────────────

class Handler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        if self.path != "/usage":
            self.send_response(404)
            self.end_headers()
            return

        with _lock:
            body = json.dumps(_cached_data, separators=(",", ":")).encode()

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        # Solo logueamos errores
        if args and str(args[1]) not in ("200", "304"):
            log(f"HTTP {self.address_string()} {fmt % args}")


# ── Punto de entrada ──────────────────────────────────────────────────────

def main() -> None:
    log("=== CYD Clawd - Servidor HTTP ===")
    log(f"Puerto: {PORT}")
    log(f"Credenciales: {CRED_FILE}")

    if not CRED_FILE.exists():
        log("ADVERTENCIA: archivo de credenciales no encontrado.")
        log("  Ejecutá Claude Code CLI al menos una vez para generarlo.")

    # Daemon de polling en hilo separado (consulta inmediata al arrancar)
    t = threading.Thread(target=run_async_loop, daemon=True)
    t.start()

    server = HTTPServer(("0.0.0.0", PORT), Handler)
    log(f"Escuchando en http://0.0.0.0:{PORT}/usage")
    log("Presioná Ctrl+C para detener")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        log("Detenido")


if __name__ == "__main__":
    main()
