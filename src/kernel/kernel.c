#include "kernel.h"
#include "../common/common.c"
#include "graphics.h"
#include "console_graphics.h"
#include "gdt.h"
#include "interrupt.h"
#include "exception.h"

int KERNEL_CALL kernel_main(kernel_args args) {
    graphics_init(args.framebuffer);    
    console_init();    

    console_set_colors(0x000000, 0xffffff);
    console_clear();
    console_printf("Kernel sagt Hallo! Er kann auch line-wrapping ohne Probleme!");

    // cli();

    // gdt_init();
    // interrupt_init();
    // exception_init();

    // sti();

    while (true) {
        asm volatile("nop");
    }

    return 0;
}