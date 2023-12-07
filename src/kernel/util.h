#pragma once
#include "../common/util.h"

#define STR(arg) _STR(arg)
#define _STR(arg) #arg

#define panic(...) _panic("KERNEL PANIC in " __FILE__ " at line " STR(__LINE__) ": " __VA_ARGS__)
void _panic(const char* fmt, ...);

#include <stdarg.h>

// Assembly operations directly callable from C code:
void cli();
void sti();
void lgdt(void* gdt, u16 limit);