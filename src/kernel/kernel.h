#pragma once
#include "graphics.h"

typedef struct {
    graphics_fb framebuffer;
    void *memory_map;
    u64 memory_map_size; 
    u64 memory_map_key;
    u64 memory_map_desc_size; 
    u32 memory_map_desc_version;
} kernel_args;

#define KERNEL_CALL __cdecl
typedef int(KERNEL_CALL *kernel_main_ptr)(kernel_args);

int KERNEL_CALL kernel_main(kernel_args args);