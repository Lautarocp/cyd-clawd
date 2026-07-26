# cyd-clawd

Claude Code usage dashboard for the ESP32-2432S028 (Cheap Yellow Display).

Shows your Claude Code rate limit consumption in real time on a 320×240 TFT screen — 5-hour session window, 7-day weekly window, and a live clock.

![dashboard showing session and weekly usage bars]()

## How it works

A lightweight Python daemon runs on your local machine (via Docker). It reads your Claude Code OAuth token from `~/.claude/.credentials.json`, polls the Anthropic API every 60 seconds with a minimal request, and extracts usage data from the response headers. The ESP32 fetches those stats over HTTP and renders them with LVGL.

```
ESP32 CYD  ──HTTP GET /usage──►  daemon (Docker)  ──HTTPS──►  api.anthropic.com
                ◄── JSON ──────────────────────────────────────────────────────
```

The daemon handles OAuth token refresh automatically — it proactively renews the token before expiry and retries on 401, so it stays online without needing Claude Code to be running.

Supports both **Pro/Max** accounts (5h + 7d windows) and **Enterprise** accounts (overage-based display).

## Hardware

- **Board:** ESP32-2432S028 (CYD — Cheap Yellow Display)
- **Display:** ILI9341 2.8" 320×240 TFT (built-in)
- **Touch:** XPT2046 resistive (built-in)

## Project structure

```
cyd-clawd/
├── daemon/
│   ├── server.py          # HTTP server + Anthropic API polling
│   └── Dockerfile
├── firmware/
│   ├── src/
│   │   ├── main.cpp       # ESP32 setup, WiFi, HTTP polling
│   │   ├── ui.cpp / ui.h  # LVGL UI
│   │   ├── theme.h        # Color palette
│   │   ├── data.h         # UsageData struct
│   │   └── config.h.example  # Copy to config.h and fill in your values
│   ├── platformio.ini
│   └── pre_build.py       # Removes ARM-only LVGL assembly files
└── docker-compose.yml
```

## Setup

### 1. Daemon (Docker)

The daemon mounts your Claude credentials as read-only and serves stats on port 8765.

```bash
cd cyd-clawd
docker compose up -d
```

Check it's working:

```bash
curl http://localhost:8765/usage
```

You should get a JSON response like:

```json
{"s":42,"sr":187,"w":18,"wr":8340,"st":"allowed","acct":"pro","ok":true,"t":1234567890,"tf":24}
```

**Requirements:** Docker with Compose, and Claude Code must have been logged in at least once on the host (so `~/.claude/.credentials.json` exists).

### 2. Firmware (ESP32)

**Prerequisites:** [PlatformIO](https://platformio.org/) (VS Code extension or CLI).

Copy the config template and fill in your values:

```bash
cp firmware/src/config.h.example firmware/src/config.h
```

Edit `firmware/src/config.h`:

```c
#define WIFI_SSID     "YourNetworkName"
#define WIFI_PASSWORD "YourPassword"
#define DAEMON_HOST   "192.168.1.100"   // IP of the machine running Docker
```

Flash the ESP32:

```bash
cd firmware
pio run --target upload
```

Monitor serial output:

```bash
pio device monitor
```

## API response fields

| Field | Type | Description |
|-------|------|-------------|
| `ok` | bool | `true` if data is valid |
| `s` | int | Session (5h) usage % |
| `sr` | int | Minutes until 5h window resets |
| `w` | int | Weekly (7d) usage % |
| `wr` | int | Minutes until 7d window resets |
| `st` | string | Rate limit status (`allowed`, `limited`, …) |
| `acct` | string | Account type: `pro` or `ent` |
| `t` | int | Local epoch timestamp (for the clock) |
| `tf` | int | Clock format: `12` or `24` |

## Notes

- `config.h` is git-ignored — never commit your WiFi credentials or local IP.
- The daemon polls with `max_tokens: 1` to minimize token consumption.
- If you're on Tailscale or have custom DNS, set `--accept-dns=false` on the host to avoid DNS issues inside Docker (the compose file already pins DNS to 8.8.8.8).
