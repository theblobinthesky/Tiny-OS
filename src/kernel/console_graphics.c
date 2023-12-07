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
    console.bg_color = 0x000000;
    console.fg_color = 0xffffff;
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

inline u32 color_blend(u32 c1, u32 c2, u8 brightness) {
    f32 t = (f32)brightness / 255.0f;
#define ext(c) ((f32)((c) & 0xff))
#define blend(a, b) (u32)((1.0f - t) * (a) + t * (b))

    f32 r1 = ext(c1), r2 = ext(c2);
    f32 g1 = ext(c1 >> 8), g2 = ext(c2 >> 8);
    f32 b1 = ext(c1 >> 16), b2 = ext(c2 >> 16);
    
    u32 blend_r = blend(r1, r2);
    u32 blend_g = blend(g1, g2);
    u32 blend_b = blend(b1, b2); 
    return (blend_b << 16) | (blend_g << 8) | blend_r;
}

static void console_put_char(char c) {
    if (c == '\n') {
        console.cursor_row++;
    } else {
        graphics_fb* fb = graphics_get_framebuffer();
        u32* fb_ptr = fb->base + (console.cursor_row * fb->width + console.cursor_col) * FONT_GLYPH_SIZE_PX;

        // Draw to framebuffer.
        // TODO: Hack.
        if (c != ' ') {    
            u8* glyph_ptr = FONT_GLYPH_PTR(c);
            for (int y = 0; y < FONT_GLYPH_SIZE_PX; y++) {
                for (int x = 0; x < FONT_GLYPH_SIZE_PX; x++) {
                    u8 glyph_brightness = glyph_ptr[y * FONT_GLYPH_SIZE_PX + x];
                    fb_ptr[y * fb->width + x] = color_blend(console.bg_color, console.fg_color, glyph_brightness);
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

    while(*fmt != 0) {
        if (*fmt == '%') {
            fmt++;
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
                    console_printf(ptr);
                } break;
                case 'c': {

                } break;
                default: {
                    panic("Invalid format argument.");   
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