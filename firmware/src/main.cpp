#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <lvgl.h>

#include "config.h"
#include "data.h"
#include "ui.h"

// ── Hardware ──────────────────────────────────────────────────────────────
#define TOUCH_CS   33
#define TOUCH_IRQ  36
#define TOUCH_CLK  25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32

static TFT_eSPI    tft;
static SPIClass    touchSPI(HSPI);
static XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// ── LVGL draw buffers (10 líneas × 320 px × 2 bytes = 6.4 KB c/u) ────────
#define BUF_LINES 10
static uint16_t buf1[320 * BUF_LINES];
static uint16_t buf2[320 * BUF_LINES];

// ── Estado de aplicación ─────────────────────────────────────────────────
static UsageData usage = {};
static unsigned long last_poll_ms = 0;

// ── LVGL callbacks ───────────────────────────────────────────────────────

static uint32_t lv_tick_cb(void) { return millis(); }

static void disp_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)px_map, w * h, true);
    tft.endWrite();
    lv_display_flush_ready(disp);
}

static void touch_read(lv_indev_t* indev, lv_indev_data_t* data) {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        data->point.x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, 320);
        data->point.y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, 240);
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// ── Parseo JSON ──────────────────────────────────────────────────────────

static bool parse_json(const char* json, UsageData* out) {
    JsonDocument doc;
    if (deserializeJson(doc, json) != DeserializationError::Ok) return false;

    out->session_pct        = doc["s"]  | 0.0f;
    out->session_reset_mins = doc["sr"] | -1;
    out->weekly_pct         = doc["w"]  | 0.0f;
    out->weekly_reset_mins  = doc["wr"] | -1;
    const char* st          = doc["st"] | "unknown";
    strlcpy(out->status, st, sizeof(out->status));
    const char* acct        = doc["acct"] | "pro";
    out->enterprise         = (strcmp(acct, "ent") == 0);
    out->clock_epoch        = doc["t"]  | 0L;
    out->clock_fmt          = doc["tf"] | 24;
    out->ok                 = doc["ok"] | false;
    out->valid              = true;
    return true;
}

// ── Polling HTTP ─────────────────────────────────────────────────────────

static void poll_daemon(void) {
    if (WiFi.status() != WL_CONNECTED) {
        ui_show_error("WiFi desconectado");
        return;
    }

    HTTPClient http;
    String url = "http://" + String(DAEMON_HOST) + ":" + String(DAEMON_PORT) + "/usage";
    http.begin(url);
    http.setTimeout(8000);

    int code = http.GET();
    if (code == 200) {
        String body = http.getString();
        if (parse_json(body.c_str(), &usage)) {
            ui_update(&usage);
            Serial.printf("[OK] s=%.0f%% w=%.0f%%\n",
                          usage.session_pct, usage.weekly_pct);
        } else {
            Serial.println("[ERR] JSON parse failed");
            ui_show_error("Error JSON");
        }
    } else {
        Serial.printf("[ERR] HTTP %d\n", code);
        ui_show_error("Error daemon");
    }
    http.end();
}

// ── Setup ─────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

    // Pantalla
    tft.begin();
    tft.setRotation(TFT_ROTATION);
    tft.fillScreen(TFT_BLACK);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // Touch (HSPI bus, pines no estándar)
    touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI);
    ts.begin(touchSPI);
    ts.setRotation(TFT_ROTATION);

    // LVGL
    lv_init();
    lv_tick_set_cb(lv_tick_cb);

    lv_display_t* disp = lv_display_create(320, 240);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read);

    ui_init();
    ui_show_connecting();
    lv_timer_handler();

    // WiFi
    Serial.printf("Conectando a %s...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
        lv_timer_handler();
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
        poll_daemon();
        last_poll_ms = millis();
    } else {
        Serial.println("WiFi timeout");
        ui_show_error("Sin WiFi");
    }
}

// ── Loop ──────────────────────────────────────────────────────────────────

void loop() {
    lv_timer_handler();

    if (millis() - last_poll_ms >= POLL_INTERVAL_MS) {
        last_poll_ms = millis();
        poll_daemon();
    }

    delay(5);
}
