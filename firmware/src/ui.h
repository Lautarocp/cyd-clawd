#pragma once
#include "data.h"

void ui_init(void);
void ui_update(const UsageData* d);
void ui_show_connecting(void);
void ui_show_error(const char* msg);
