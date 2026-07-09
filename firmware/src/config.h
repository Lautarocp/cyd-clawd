#pragma once

// ── WiFi ──────────────────────────────────────────────────────────────────
#define WIFI_SSID     "Movistarcasa"
#define WIFI_PASSWORD "01234567890"

// ── Daemon HTTP ───────────────────────────────────────────────────────────
#define DAEMON_HOST   "192.168.2.169"
#define DAEMON_PORT   8765
#define POLL_INTERVAL_MS 60000   // consulta cada 60 segundos

// ── Pantalla ──────────────────────────────────────────────────────────────
// Rotación: 1 = landscape (320x240) con USB a la derecha
#define TFT_ROTATION  1

// ── Touch calibración ─────────────────────────────────────────────────────
// Si el toque no es preciso, ajustá estos valores con el sketch de calibración
#define TOUCH_X_MIN  200
#define TOUCH_X_MAX  3700
#define TOUCH_Y_MIN  240
#define TOUCH_Y_MAX  3800
