#pragma once
#include <Arduino.h>

struct UsageData {
    float session_pct;       // 0–100 (ventana 5h)
    int   session_reset_mins;
    float weekly_pct;        // 0–100 (ventana 7d)
    int   weekly_reset_mins;
    char  status[16];        // "allowed", "limited", …
    bool  enterprise;
    long  clock_epoch;       // epoch local del daemon (0 = no disponible)
    int   clock_fmt;         // 12 o 24
    bool  ok;
    bool  valid;
};
