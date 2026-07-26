#pragma once
#include <Arduino.h>

struct UsageData {
    float session_pct;
    int   session_reset_mins;
    float weekly_pct;
    int   weekly_reset_mins;
    char  status[16];
    bool  enterprise;
    long  clock_epoch;
    int   clock_fmt;
    bool  ok;
    bool  valid;
};

struct SysData {
    int  cpu_pct;
    int  ram_used_mb;
    int  ram_total_mb;
    int  disk_used_gb;
    int  disk_total_gb;
    long clock_epoch;
    int  clock_fmt;
    bool ok;
    bool valid;
};
