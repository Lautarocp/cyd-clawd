#include "ui.h"
#include "theme.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

// Dimensiones del CYD en landscape
#define SCR_W 320
#define SCR_H 240

// Alturas de las zonas
#define HDR_H  28
#define PANEL_H 92
#define PANEL_GAP 4
#define STATUS_H 20

// ── Widgets persistentes ─────────────────────────────────────────────────
static lv_obj_t* scr;

// Header
static lv_obj_t* lbl_title;
static lv_obj_t* lbl_clock;

// Panel sesión (5h)
static lv_obj_t* panel_s;
static lv_obj_t* lbl_s_title;
static lv_obj_t* lbl_s_pct;
static lv_obj_t* bar_s;
static lv_obj_t* lbl_s_reset;

// Panel semanal (7d)
static lv_obj_t* panel_w;
static lv_obj_t* lbl_w_title;
static lv_obj_t* lbl_w_pct;
static lv_obj_t* bar_w;
static lv_obj_t* lbl_w_reset;

// Barra de estado inferior
static lv_obj_t* lbl_status;

// ── Helpers ──────────────────────────────────────────────────────────────

static lv_color_t bar_color(float pct) {
    if (pct >= 80.0f) return THEME_RED;
    if (pct >= 50.0f) return THEME_AMBER;
    return THEME_GREEN;
}

// Formatea minutos como "2h 15m" o "3d 4h"
static void fmt_reset(char* buf, size_t n, int mins) {
    if (mins < 0) {
        snprintf(buf, n, "--");
    } else if (mins < 60) {
        snprintf(buf, n, "%dm", mins);
    } else if (mins < 60 * 24) {
        snprintf(buf, n, "%dh %02dm", mins / 60, mins % 60);
    } else {
        int d = mins / (60 * 24);
        int h = (mins % (60 * 24)) / 60;
        snprintf(buf, n, "%dd %dh", d, h);
    }
}

static lv_obj_t* make_panel(int y) {
    lv_obj_t* p = lv_obj_create(scr);
    lv_obj_set_size(p, SCR_W - 8, PANEL_H);
    lv_obj_set_pos(p, 4, y);
    lv_obj_set_style_bg_color(p, THEME_PANEL, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(p, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(p, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(p, 0, LV_PART_MAIN);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

static lv_obj_t* make_bar(lv_obj_t* parent, int y) {
    lv_obj_t* b = lv_bar_create(parent);
    lv_obj_set_size(b, SCR_W - 8 - 16, 10);
    lv_obj_set_pos(b, 8, y);
    lv_bar_set_range(b, 0, 100);
    lv_obj_set_style_bg_color(b, THEME_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(b, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(b, 3, LV_PART_INDICATOR);
    return b;
}

static lv_obj_t* make_label(lv_obj_t* parent, int x, int y,
                             const lv_font_t* font, lv_color_t color,
                             const char* text) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, color, LV_PART_MAIN);
    lv_label_set_text(l, text);
    return l;
}

// ── API pública ───────────────────────────────────────────────────────────

void ui_init(void) {
    scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, THEME_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ── Header ─────────────────────────────────────────────────────────
    lbl_title = make_label(scr, 8, 6,
                           &lv_font_montserrat_14, THEME_ACCENT,
                           "Claude Code Usage");
    lbl_clock = make_label(scr, SCR_W - 50, 6,
                           &lv_font_montserrat_14, THEME_DIM, "--:--");

    // Línea separadora header
    lv_obj_t* sep = lv_obj_create(scr);
    lv_obj_set_size(sep, SCR_W, 1);
    lv_obj_set_pos(sep, 0, HDR_H);
    lv_obj_set_style_bg_color(sep, THEME_PANEL, LV_PART_MAIN);
    lv_obj_set_style_border_width(sep, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(sep, 0, LV_PART_MAIN);

    // ── Panel sesión (5h) ───────────────────────────────────────────────
    int p1_y = HDR_H + 3;
    panel_s = make_panel(p1_y);

    lbl_s_title = make_label(panel_s, 8, 6,
                             &lv_font_montserrat_12, THEME_DIM, "5h Session");
    lbl_s_pct = make_label(panel_s, 8, 22,
                           &lv_font_montserrat_32, THEME_TEXT, "0%");
    bar_s = make_bar(panel_s, 65);
    lv_obj_set_style_bg_color(bar_s, THEME_GREEN, LV_PART_INDICATOR);
    lv_bar_set_value(bar_s, 0, LV_ANIM_OFF);

    lbl_s_reset = make_label(panel_s, 8, 78,
                             &lv_font_montserrat_12, THEME_DIM, "Reset: --");

    // ── Panel semanal (7d) ──────────────────────────────────────────────
    int p2_y = p1_y + PANEL_H + PANEL_GAP;
    panel_w = make_panel(p2_y);

    lbl_w_title = make_label(panel_w, 8, 6,
                             &lv_font_montserrat_12, THEME_DIM, "7d Weekly");
    lbl_w_pct = make_label(panel_w, 8, 22,
                           &lv_font_montserrat_32, THEME_TEXT, "0%");
    bar_w = make_bar(panel_w, 65);
    lv_obj_set_style_bg_color(bar_w, THEME_GREEN, LV_PART_INDICATOR);
    lv_bar_set_value(bar_w, 0, LV_ANIM_OFF);

    lbl_w_reset = make_label(panel_w, 8, 78,
                             &lv_font_montserrat_12, THEME_DIM, "Reset: --");

    // ── Barra de estado inferior ────────────────────────────────────────
    int status_y = p2_y + PANEL_H + PANEL_GAP;
    lbl_status = make_label(scr, 8, status_y + 3,
                            &lv_font_montserrat_12, THEME_DIM,
                            "Conectando...");
}

void ui_update(const UsageData* d) {
    if (!d || !d->valid) return;

    // ── Reloj ───────────────────────────────────────────────────────────
    if (d->clock_epoch > 0) {
        long t = d->clock_epoch;
        int h = (t / 3600) % 24;
        int m = (t / 60) % 60;
        char cbuf[8];
        if (d->clock_fmt == 12) {
            const char* am = h >= 12 ? "PM" : "AM";
            h = h % 12;
            if (h == 0) h = 12;
            snprintf(cbuf, sizeof(cbuf), "%d:%02d%s", h, m, am);
        } else {
            snprintf(cbuf, sizeof(cbuf), "%02d:%02d", h, m);
        }
        lv_label_set_text(lbl_clock, cbuf);
    }

    // ── Panel sesión ────────────────────────────────────────────────────
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", (int)d->session_pct);
        lv_label_set_text(lbl_s_pct, buf);
        lv_obj_set_style_text_color(lbl_s_pct, bar_color(d->session_pct), LV_PART_MAIN);

        lv_bar_set_value(bar_s, (int)d->session_pct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar_s, bar_color(d->session_pct), LV_PART_INDICATOR);

        char rbuf[16];
        fmt_reset(rbuf, sizeof(rbuf), d->session_reset_mins);
        char reset[24];
        snprintf(reset, sizeof(reset), "Reset: %s", rbuf);
        lv_label_set_text(lbl_s_reset, reset);
    }

    // ── Panel semanal ───────────────────────────────────────────────────
    if (d->enterprise) {
        lv_label_set_text(lbl_w_title, "Period");
        lv_label_set_text(lbl_w_pct, "N/A");
        lv_bar_set_value(bar_w, 0, LV_ANIM_OFF);
        lv_label_set_text(lbl_w_reset, "Enterprise");
    } else {
        lv_label_set_text(lbl_w_title, "7d Weekly");
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", (int)d->weekly_pct);
        lv_label_set_text(lbl_w_pct, buf);
        lv_obj_set_style_text_color(lbl_w_pct, bar_color(d->weekly_pct), LV_PART_MAIN);

        lv_bar_set_value(bar_w, (int)d->weekly_pct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar_w, bar_color(d->weekly_pct), LV_PART_INDICATOR);

        char rbuf[16];
        fmt_reset(rbuf, sizeof(rbuf), d->weekly_reset_mins);
        char reset[24];
        snprintf(reset, sizeof(reset), "Reset: %s", rbuf);
        lv_label_set_text(lbl_w_reset, reset);
    }

    // ── Status ──────────────────────────────────────────────────────────
    const char* st = d->status;
    if (strcmp(st, "allowed") == 0) {
        lv_label_set_text(lbl_status, "OK");
        lv_obj_set_style_text_color(lbl_status, THEME_GREEN, LV_PART_MAIN);
    } else if (strcmp(st, "limited") == 0) {
        lv_label_set_text(lbl_status, "Rate limited");
        lv_obj_set_style_text_color(lbl_status, THEME_RED, LV_PART_MAIN);
    } else {
        lv_label_set_text(lbl_status, st);
        lv_obj_set_style_text_color(lbl_status, THEME_DIM, LV_PART_MAIN);
    }
}

void ui_show_connecting(void) {
    if (lbl_status) {
        lv_label_set_text(lbl_status, "Conectando a WiFi...");
        lv_obj_set_style_text_color(lbl_status, THEME_DIM, LV_PART_MAIN);
    }
}

void ui_show_error(const char* msg) {
    if (lbl_status) {
        lv_label_set_text(lbl_status, msg);
        lv_obj_set_style_text_color(lbl_status, THEME_RED, LV_PART_MAIN);
    }
}
