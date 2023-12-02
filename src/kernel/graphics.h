#pragma once
#include "util.h"

typedef struct {
    u32* base;
    u32 size;
    int width;
    int stride;
    int height;
} graphics_fb;

void graphics_init(graphics_fb _fb);
void graphics_fill_rect(int x, int y, int width, int height, u32 color);
void graphics_set_pixel(int x, int y, u32 color);
graphics_fb* graphics_get_framebuffer();