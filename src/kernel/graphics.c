#include "graphics.h"

graphics_fb fb;

void graphics_init(graphics_fb _fb) {
    fb = _fb;
}

void graphics_fill_rect(int x, int y, int width, int height, u32 color) {
    if (x + width > fb.width || y + height > fb.height) {
        panic("Tried to draw rectangle outside of framebuffer.");
        return; // not strictly necessary
    }

    for (int py = y; py < y + height; py++) {
        for (int px = x; px < x + width; px++) {
            fb.base[fb.width * py + px] = color;
        }
    }
}

void graphics_set_pixel(int x, int y, u32 color) {
    fb.base[fb.width * y + x] = color;
}

graphics_fb* graphics_get_framebuffer() {
    return &fb;    
}