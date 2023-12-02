#pragma once
#include "util.h"

void console_init();
void console_set_colors(u32 bg, u32 fg);

void console_clear();
void console_vprintf(const char* fmt, va_list args);
void console_printf(const char* fmt, ...);