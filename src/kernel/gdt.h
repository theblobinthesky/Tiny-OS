#pragma once
#include "util.h"

#define KERNEL_CODE_SEGMENT_DESC 8
#define KERNEL_DATA_SEGMENT_DESC (2*8)
// #define USER_CODE_SEGMENT_DESC (3*8)
// #define USER_DATA_SEGMENT_DESC (4*8)

void gdt_init();