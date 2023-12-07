#include "util.h"
#include "console_graphics.h"

void _panic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    console_set_colors(0x0000aa, 0xffffff);
    console_clear();
    console_vprintf(fmt, args);
    console_printf("\n");

    va_end(args);
    
    while(true) {
        asm volatile("nop");
    }
}

void cli() {
    asm volatile("cli");
}

void sti() {
    asm volatile("sti");
}
  
struct gdt_desc {
    u16 limit;
    void* offset;
} __attribute__((packed));

void lgdt(void* gdt, u16 limit) {
    struct gdt_desc desc;
    desc.offset = gdt;
    desc.limit = limit;

    asm volatile("lgdt %0" :: "m" (desc));
}