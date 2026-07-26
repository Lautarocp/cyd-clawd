# cyd-clawd

Claude Code usage dashboard for the ESP32-2432S028 (Cheap Yellow Display).

Shows your Claude Code rate limit consumption and host system stats in real time on a 320×240 TFT touchscreen. Tap anywhere to switch between screens.

![dashboard showing session and weekly usage bars]()

## Screens

### Screen 1 — Claude Code Usage
- **5h Session** usage bar with percentage and time until reset
- **7d Weekly** usage bar with percentage and time until reset
- Live clock (top right)
- Rate limit status indicator

### Screen 2 — System Stats
- **CPU** usage % with color bar
- **RAM** used / total (auto-scaled MB or GB)
- **Disk** used / total (GB)
- Live clock (top right)

Tap anywhere on the screen to slide between screens.

## How it works

A lightweight Python daemon runs on your local machine (via Docker). It reads your Claude Code OAuth token from `~/.claude/.credentials.json`, polls the Anthropic API every 60 seconds, and extracts usage data from the response headers. It also exposes host system stats via `psutil`. The ESP32 fetches data over HTTP and renders both screens with LVGL.

```
ESP32 CYD  ──HTTP GET /usage──►  daemon (Docker)  ──HTTPS──►  api.anthropic.com
           ──HTTP GET /sys───►   daemon (Docker)  ──────────►  psutil (host)
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
│   ├── server.py          # HTTP server, Anthropic API polling, /sys stats
│   └── Dockerfile
├── firmware/
│   ├── src/
│   │   ├── main.cpp       # ESP32 setup, WiFi, HTTP polling, screen tracking
│   │   ├── ui.cpp / ui.h  # LVGL UI — two screens + touch navigation
│   │   ├── theme.h        # Color palette
│   │   ├── data.h         # UsageData and SysData structs
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

Check both endpoints are working:

```bash
curl http://localhost:8765/usage
curl http://localhost:8765/sys
```

Expected responses:

```json
{"s":42,"sr":187,"w":18,"wr":8340,"st":"allowed","acct":"pro","ok":true,"t":1234567890,"tf":24}
{"cpu":12,"rmu":3276,"rmt":8192,"dku":45,"dkt":240,"ok":true,"t":1234567890,"tf":24}
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

## API endpoints

### `GET /usage` — Claude Code stats

Polled every 60 seconds by the ESP32.

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

### `GET /sys` — Host system stats

Polled every 5 seconds while the System Stats screen is active. Uses `psutil` to read the host machine (not the Docker container's view).

| Field | Type | Description |
|-------|------|-------------|
| `ok` | bool | `true` if data is valid |
| `cpu` | int | CPU usage % |
| `rmu` | int | RAM used (MB) |
| `rmt` | int | RAM total (MB) |
| `dku` | int | Disk used (GB) — root filesystem |
| `dkt` | int | Disk total (GB) — root filesystem |
| `t` | int | Local epoch timestamp (for the clock) |
| `tf` | int | Clock format: `12` or `24` |

## Touch navigation

The XPT2046 touch controller is wired to LVGL as a pointer input device. Each screen has a full-screen transparent overlay (`lv_obj`) that captures tap events and triggers `lv_scr_load_anim` with a left/right slide animation (250ms). No gesture libraries needed.

- **Tap** → switch to next screen (cycles: Usage → System Stats → Usage …)

## Polling behaviour

| Screen | `/usage` | `/sys` |
|--------|----------|--------|
| Claude Code Usage | every 60s | not polled |
| System Stats | every 60s | immediately on switch, then every 5s |

`/usage` is always polled so data is fresh when switching back to screen 1.

## Notes

- `config.h` is git-ignored — never commit your WiFi credentials or local IP.
- The daemon polls with `max_tokens: 1` to minimize token consumption.
- `/sys` reads the **host** filesystem at `/`. If you want to monitor a different mount point, edit `disk_usage('/')` in `server.py`.
- If you're on Tailscale or have custom DNS, set `--accept-dns=false` on the host to avoid DNS issues inside Docker (the compose file already pins DNS to 8.8.8.8).
