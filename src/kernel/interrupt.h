#pragma once
#include "util.h"

typedef void(*interrupt_handler)(u64 error_code);

void interrupt_init();
void set_interrupt_handler(u32 interrupt_index, interrupt_handler handler);