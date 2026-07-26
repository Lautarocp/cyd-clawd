#pragma once
#include "data.h"

void ui_init(void);
void ui_update(const UsageData* d);
void ui_show_connecting(void);
void ui_show_error(const char* msg);

void ui_sys_init(void);
void ui_sys_update(const SysData* d);
void ui_sys_show_error(const char* msg);

int  ui_current_screen(void);
