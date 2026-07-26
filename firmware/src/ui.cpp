#include "ui.h"
#include "theme.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#define SCR_W    320
#define SCR_H    240
#define HDR_H     28
#define PANEL_H   92
#define PANEL_GAP  4

// ── Screens ───────────────────────────────────────────────────────────────
static lv_obj_t* scr_usage;
static lv_obj_t* scr_sys;
static int g_screen = 0;

// ── Usage screen widgets ──────────────────────────────────────────────────
static lv_obj_t* lbl_title;
static lv_obj_t* lbl_clock;
static lv_obj_t* panel_s;
static lv_obj_t* lbl_s_title;
static lv_obj_t* lbl_s_pct;
static lv_obj_t* bar_s;
static lv_obj_t* lbl_s_reset;
static lv_obj_t* panel_w;
static lv_obj_t* lbl_w_title;
static lv_obj_t* lbl_w_pct;
static lv_obj_t* bar_w;
static lv_obj_t* lbl_w_reset;
static lv_obj_t* lbl_status;

// ── Sys screen widgets ────────────────────────────────────────────────────
static lv_obj_t* lbl_sys_title;
static lv_obj_t* lbl_sys_clock;
static lv_obj_t* panel_cpu;
static lv_obj_t* lbl_cpu_pct;
static lv_obj_t* bar_cpu;
static lv_obj_t* panel_mem;
static lv_obj_t* lbl_ram_val;
static lv_obj_t* bar_ram;
static lv_obj_t* lbl_disk_val;
static lv_obj_t* bar_disk;
static lv_obj_t* lbl_sys_status;

// ── Helpers ───────────────────────────────────────────────────────────────

static lv_color_t bar_color(float pct) {
    if (pct >= 80.0f) return THEME_RED;
    if (pct >= 50.0f) return THEME_AMBER;
    return THEME_GREEN;
}

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

static void fmt_clock(char* buf, size_t n, long epoch, int fmt) {
    int h = (epoch / 3600) % 24;
    int m = (epoch / 60)   % 60;
    if (fmt == 12) {
        const char* am = h >= 12 ? "PM" : "AM";
        h = h % 12;
        if (h == 0) h = 12;
        snprintf(buf, n, "%d:%02d%s", h, m, am);
    } else {
        snprintf(buf, n, "%02d:%02d", h, m);
    }
}

static void fmt_memory(char* buf, size_t n, int used_mb, int total_mb) {
    if (total_mb >= 1024) {
        snprintf(buf, n, "%.1f / %.1f GB", used_mb / 1024.0f, total_mb / 1024.0f);
    } else {
        snprintf(buf, n, "%d / %d MB", used_mb, total_mb);
    }
}

static lv_obj_t* make_panel(lv_obj_t* parent, int y) {
    lv_obj_t* p = lv_obj_create(parent);
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

// Overlay transparente full-screen para capturar taps en toda la pantalla.
static void make_tap_overlay(lv_obj_t* scr, lv_event_cb_t cb) {
    lv_obj_t* o = lv_obj_create(scr);
    lv_obj_set_size(o, SCR_W, SCR_H);
    lv_obj_set_pos(o, 0, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(o, 0, LV_PART_MAIN);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(o, cb, LV_EVENT_CLICKED, NULL);
}

// ── Navegación entre pantallas ────────────────────────────────────────────

static void on_tap(lv_event_t* e) {
    (void)e;
    g_screen = (g_screen + 1) % 2;
    lv_obj_t* target = (g_screen == 0) ? scr_usage : scr_sys;
    lv_scr_load_anim_t dir = (g_screen == 1) ? LV_SCR_LOAD_ANIM_MOVE_LEFT
                                              : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    lv_scr_load_anim(target, dir, 250, 0, false);
}

int ui_current_screen(void) { return g_screen; }

// ── Usage screen ──────────────────────────────────────────────────────────

void ui_init(void) {
    scr_usage = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_usage, THEME_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr_usage, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(scr_usage, LV_OBJ_FLAG_SCROLLABLE);

    lbl_title = make_label(scr_usage, 8, 6,
                           &lv_font_montserrat_14, THEME_ACCENT,
                           "Claude Code Usage");
    lbl_clock = make_label(scr_usage, SCR_W - 50, 6,
                           &lv_font_montserrat_14, THEME_DIM, "--:--");

    lv_obj_t* sep = lv_obj_create(scr_usage);
    lv_obj_set_size(sep, SCR_W, 1);
    lv_obj_set_pos(sep, 0, HDR_H);
    lv_obj_set_style_bg_color(sep, THEME_PANEL, LV_PART_MAIN);
    lv_obj_set_style_border_width(sep, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(sep, 0, LV_PART_MAIN);

    int p1_y = HDR_H + 3;
    panel_s = make_panel(scr_usage, p1_y);
    lbl_s_title = make_label(panel_s, 8, 6,
                             &lv_font_montserrat_12, THEME_DIM, "5h Session");
    lbl_s_pct = make_label(panel_s, 8, 22,
                           &lv_font_montserrat_32, THEME_TEXT, "0%");
    bar_s = make_bar(panel_s, 65);
    lv_obj_set_style_bg_color(bar_s, THEME_GREEN, LV_PART_INDICATOR);
    lv_bar_set_value(bar_s, 0, LV_ANIM_OFF);
    lbl_s_reset = make_label(panel_s, 8, 78,
                             &lv_font_montserrat_12, THEME_DIM, "Reset: --");

    int p2_y = p1_y + PANEL_H + PANEL_GAP;
    panel_w = make_panel(scr_usage, p2_y);
    lbl_w_title = make_label(panel_w, 8, 6,
                             &lv_font_montserrat_12, THEME_DIM, "7d Weekly");
    lbl_w_pct = make_label(panel_w, 8, 22,
                           &lv_font_montserrat_32, THEME_TEXT, "0%");
    bar_w = make_bar(panel_w, 65);
    lv_obj_set_style_bg_color(bar_w, THEME_GREEN, LV_PART_INDICATOR);
    lv_bar_set_value(bar_w, 0, LV_ANIM_OFF);
    lbl_w_reset = make_label(panel_w, 8, 78,
                             &lv_font_montserrat_12, THEME_DIM, "Reset: --");

    int status_y = p2_y + PANEL_H + PANEL_GAP;
    lbl_status = make_label(scr_usage, 8, status_y + 3,
                            &lv_font_montserrat_12, THEME_DIM, "Conectando...");

    make_tap_overlay(scr_usage, on_tap);
    lv_scr_load(scr_usage);
}

void ui_update(const UsageData* d) {
    if (!d || !d->valid) return;

    if (d->clock_epoch > 0) {
        char cbuf[8];
        fmt_clock(cbuf, sizeof(cbuf), d->clock_epoch, d->clock_fmt);
        lv_label_set_text(lbl_clock, cbuf);
    }

    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", (int)d->session_pct);
        lv_label_set_text(lbl_s_pct, buf);
        lv_obj_set_style_text_color(lbl_s_pct, bar_color(d->session_pct), LV_PART_MAIN);
        lv_bar_set_value(bar_s, (int)d->session_pct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar_s, bar_color(d->session_pct), LV_PART_INDICATOR);
        char rbuf[16], reset[24];
        fmt_reset(rbuf, sizeof(rbuf), d->session_reset_mins);
        snprintf(reset, sizeof(reset), "Reset: %s", rbuf);
        lv_label_set_text(lbl_s_reset, reset);
    }

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
        char rbuf[16], reset[24];
        fmt_reset(rbuf, sizeof(rbuf), d->weekly_reset_mins);
        snprintf(reset, sizeof(reset), "Reset: %s", rbuf);
        lv_label_set_text(lbl_w_reset, reset);
    }

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

// ── Sys screen ────────────────────────────────────────────────────────────

void ui_sys_init(void) {
    scr_sys = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_sys, THEME_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr_sys, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(scr_sys, LV_OBJ_FLAG_SCROLLABLE);

    lbl_sys_title = make_label(scr_sys, 8, 6,
                               &lv_font_montserrat_14, THEME_ACCENT,
                               "System Stats");
    lbl_sys_clock = make_label(scr_sys, SCR_W - 50, 6,
                               &lv_font_montserrat_14, THEME_DIM, "--:--");

    lv_obj_t* sep = lv_obj_create(scr_sys);
    lv_obj_set_size(sep, SCR_W, 1);
    lv_obj_set_pos(sep, 0, HDR_H);
    lv_obj_set_style_bg_color(sep, THEME_PANEL, LV_PART_MAIN);
    lv_obj_set_style_border_width(sep, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(sep, 0, LV_PART_MAIN);

    // Panel CPU
    int p1_y = HDR_H + 3;
    panel_cpu = make_panel(scr_sys, p1_y);
    make_label(panel_cpu, 8, 6, &lv_font_montserrat_12, THEME_DIM, "CPU");
    lbl_cpu_pct = make_label(panel_cpu, 8, 22,
                             &lv_font_montserrat_32, THEME_TEXT, "0%");
    bar_cpu = make_bar(panel_cpu, 65);
    lv_obj_set_style_bg_color(bar_cpu, THEME_GREEN, LV_PART_INDICATOR);
    lv_bar_set_value(bar_cpu, 0, LV_ANIM_OFF);

    // Panel RAM + Disk
    int p2_y = p1_y + PANEL_H + PANEL_GAP;
    panel_mem = make_panel(scr_sys, p2_y);

    make_label(panel_mem, 8, 4,  &lv_font_montserrat_12, THEME_DIM, "RAM");
    lbl_ram_val = make_label(panel_mem, 8, 18,
                             &lv_font_montserrat_16, THEME_TEXT, "-- / -- GB");
    bar_ram = make_bar(panel_mem, 36);
    lv_obj_set_style_bg_color(bar_ram, THEME_GREEN, LV_PART_INDICATOR);
    lv_bar_set_value(bar_ram, 0, LV_ANIM_OFF);

    make_label(panel_mem, 8, 50, &lv_font_montserrat_12, THEME_DIM, "Disk");
    lbl_disk_val = make_label(panel_mem, 8, 64,
                              &lv_font_montserrat_16, THEME_TEXT, "-- / -- GB");
    bar_disk = make_bar(panel_mem, 82);
    lv_obj_set_style_bg_color(bar_disk, THEME_GREEN, LV_PART_INDICATOR);
    lv_bar_set_value(bar_disk, 0, LV_ANIM_OFF);

    int status_y = p2_y + PANEL_H + PANEL_GAP;
    lbl_sys_status = make_label(scr_sys, 8, status_y + 3,
                                &lv_font_montserrat_12, THEME_DIM, "Esperando...");

    make_tap_overlay(scr_sys, on_tap);
}

void ui_sys_update(const SysData* d) {
    if (!d || !d->valid) return;

    if (d->clock_epoch > 0) {
        char cbuf[8];
        fmt_clock(cbuf, sizeof(cbuf), d->clock_epoch, d->clock_fmt);
        lv_label_set_text(lbl_sys_clock, cbuf);
    }

    // CPU
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", d->cpu_pct);
        lv_label_set_text(lbl_cpu_pct, buf);
        lv_obj_set_style_text_color(lbl_cpu_pct, bar_color((float)d->cpu_pct), LV_PART_MAIN);
        lv_bar_set_value(bar_cpu, d->cpu_pct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar_cpu, bar_color((float)d->cpu_pct), LV_PART_INDICATOR);
    }

    // RAM
    {
        int pct = (d->ram_total_mb > 0) ? (d->ram_used_mb * 100 / d->ram_total_mb) : 0;
        char buf[24];
        fmt_memory(buf, sizeof(buf), d->ram_used_mb, d->ram_total_mb);
        lv_label_set_text(lbl_ram_val, buf);
        lv_bar_set_value(bar_ram, pct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar_ram, bar_color((float)pct), LV_PART_INDICATOR);
    }

    // Disk
    {
        int pct = (d->disk_total_gb > 0) ? (d->disk_used_gb * 100 / d->disk_total_gb) : 0;
        char buf[24];
        snprintf(buf, sizeof(buf), "%d / %d GB", d->disk_used_gb, d->disk_total_gb);
        lv_label_set_text(lbl_disk_val, buf);
        lv_bar_set_value(bar_disk, pct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar_disk, bar_color((float)pct), LV_PART_INDICATOR);
    }

    if (d->ok) {
        lv_label_set_text(lbl_sys_status, "OK");
        lv_obj_set_style_text_color(lbl_sys_status, THEME_GREEN, LV_PART_MAIN);
    } else {
        lv_label_set_text(lbl_sys_status, "Sin datos");
        lv_obj_set_style_text_color(lbl_sys_status, THEME_RED, LV_PART_MAIN);
    }
}

void ui_sys_show_error(const char* msg) {
    if (lbl_sys_status) {
        lv_label_set_text(lbl_sys_status, msg);
        lv_obj_set_style_text_color(lbl_sys_status, THEME_RED, LV_PART_MAIN);
    }
}
