#include "kernel.h"
#include "../common/common.c"
#include "graphics.h"
#include "console_graphics.h"
#include "gdt.h"
#include "interrupt.h"
#include "virtual_memory.h"

int KERNEL_CALL kernel_main(kernel_args args) {
    graphics_init(args.framebuffer);    
    console_init();    

    console_set_colors(0x000000, 0xffffff);
    console_clear();

    cli();

    gdt_init();
    interrupt_init();

    sti();

    console_printf("Kernel sagt Hallo! Er kann auch line-wrapping ohne Probleme!");

    vm_init(args.memory_map, args.memory_map_size, args.memory_map_desc_size);
    char* page = (char *)vm_palloc(1);
    const char *str = "abcdefghijklmnopqrstuvw";
    for (int i = 0; i < 24; i++) {
        page[i] = str[i];
    }
    console_printf(page);
    vm_pfree(page, 1);

    while (true) {
        asm volatile("nop");
    }

    return 0;
}