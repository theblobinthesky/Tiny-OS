#include "console_graphics.h"
#include "graphics.h"
#include "pixel_font.h"

#define CURSOR_PADDING 1

struct {
    u32 cursor_col;
    u32 cursor_row;
    u32 chars_per_row;
    u32 bg_color;
    u32 fg_color;
} console;

void console_init() {
    graphics_fb* fb = graphics_get_framebuffer();

    console.cursor_col = CURSOR_PADDING;
    console.cursor_row = CURSOR_PADDING;
    console.chars_per_row = fb->width / FONT_GLYPH_SIZE_PX;
    console.bg_color = 0x00000000;
    console.fg_color = 0x00ffffff;
}

void console_set_colors(u32 bg, u32 fg) {
    console.bg_color = bg;
    console.fg_color = fg;
}

void console_clear() {
    graphics_fb* fb = graphics_get_framebuffer();
    for (int i = 0; i < fb->height * fb->width; i++) {
        fb->base[i] = console.bg_color;
    }
}

static void console_put_char(char c) {
    graphics_fb* fb = graphics_get_framebuffer();
    u32* fb_ptr = fb->base + (console.cursor_row * fb->width + console.cursor_col) * FONT_GLYPH_SIZE_PX;

    // Draw to framebuffer.
    // TODO: Hack.
    if (c != ' ') {    
        char* glyph_ptr = FONT_GLYPH_PTR(c);
        for (int y = 0; y < FONT_GLYPH_SIZE_PX; y++) {
            for (int x = 0; x < FONT_GLYPH_SIZE_PX; x++) {
                fb_ptr[y * fb->width + x] = glyph_ptr[y * FONT_GLYPH_SIZE_PX + x];
            }
        }    
    }

    // Finally update the cursor position, taking into account the cursor padding.    
    if (console.cursor_col + 2 * CURSOR_PADDING + 1 > console.chars_per_row) {
        console.cursor_col = CURSOR_PADDING;
        console.cursor_row++;
    } else {
        console.cursor_col++;
    }
}

static void sprint_int(char* buffer, int value, int base) {
    int i = 0;
    const char* digits = "0123456789ABCDEF";

    if (value == 0) {
        buffer[i++] = digits[0];
    } else {
        int len = 0;
        for (int temp = value; temp > 0; temp /= base) len++;

        for (int j = i + len - 1; j >= i; j--) {
            char digit = digits[value % base];
            value /= base;
            buffer[j] = digit;
        }

        i += len;
    }

    buffer[i++] = 0;
}

void console_vprintf(const char* fmt, va_list args) {
    char str_buffer[32];

    while(*fmt) {
        if (*fmt == '%') {
            switch(*fmt) {
                case 'x': {
                    u64 v = va_arg(args, u64);                    
                    sprint_int(str_buffer, v, 16);
                    console_printf(str_buffer);
                } break;
                case 'd': {
                    s64 v = va_arg(args, s64);                    
                    if (v < 0) {
                        console_put_char('-');
                        v = -v;
                    }

                    sprint_int(str_buffer, v, 10);
                    console_printf(str_buffer);
                } break;
                case 'u': {
                    u64 v = va_arg(args, u64);
                    sprint_int(str_buffer, v, 10);
                    console_printf(str_buffer);
                } break;
                case 's': {
                    char* ptr = va_arg(args, char*);
                    
                } break;
                case 'c': {

                } break;
                default: {
                    panic("Tried to argument of unknown type.");   
                } return;
            }
        } else {
            console_put_char(*fmt);
        }

        fmt++;
    }
}

void console_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    console_vprintf(fmt, args);

    va_end(args);
}