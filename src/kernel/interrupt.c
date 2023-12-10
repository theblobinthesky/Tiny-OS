#include "interrupt.h"
#include "gdt.h"

typedef struct {
   u16 offset_1; // offset bits 0..15
   u16 selector; // code segment selector
   u8 ist : 3; // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
   u8 zero_1 : 5;
   u8 gate_type : 4;
   u8 zero_2 : 1;
   u8 dpl : 2;
   u8 present : 1;
   u16 offset_2; // offset bits 16..31
   u32 offset_3; // offset bits 32..63
   u32 zero_3;
} __attribute__((packed)) interrupt_desc_64;

typedef struct {
    u16 size;
    u64 offset;
} __attribute__((packed)) interrupt_table_desc;

#define IDT_SIZE 256
interrupt_desc_64 idt[IDT_SIZE];
interrupt_table_desc idt_desc;
interrupt_handler handlers[IDT_SIZE];

void handler_div_by_zero(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_debug(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_nmi(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_breakpoint(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_overflow(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_bound_range_exceeded(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_invalid_opcode(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_device_not_available(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_double_fault(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_invalid_tss(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_segment_not_present(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_stack_segment_fault(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_general_protection_fault(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_page_fault(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_x87_fp_exeption(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_alignment_check(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_machine_check(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_simd_fp_exception(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_virtualization_exception(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_security_exception(u64 error_code) {
    panic("Error code %u", error_code);
}

void handler_no_ec_default(u64 error_code) {
    panic("Error code %u", error_code);
}

void set_x64_interrupt_service_routine(u32 interrupt_index, void(*service_routine)) {
    u64 isr_addr = (u64)service_routine;
    interrupt_desc_64 desc = {
        .offset_1 = (u16)(isr_addr & 0xffff),
        .selector = KERNEL_CODE_SEGMENT_DESC,
        .ist = 0,
        .gate_type = 0xF, // Trap
        .dpl = 0,
        .present = 1,
        .offset_2 = (u16)((isr_addr >> 16) & 0xffff),
        .offset_3 = (u32)((isr_addr >> 32) & 0xffffffff),
    };

    idt[interrupt_index] = desc;
}

void interrupt_init() {
    for (int i = 0; i < IDT_SIZE; i++) {
        // initialize to default values
        void isr_no_ec_default();
        set_interrupt_handler(i, handler_no_ec_default);
        set_x64_interrupt_service_routine(i, isr_no_ec_default);
    }

    #define INTERRUPT_TABLE_C
    #include "interrupt_table.h"
    #undef INTERRUPT_TABLE_C

    idt_desc.size = sizeof(idt) - 1;
    idt_desc.offset = (u64)idt;

    asm volatile("lidt %0" : : "m" (idt_desc));
}

void set_interrupt_handler(u32 interrupt_index, interrupt_handler handler) {
    if (interrupt_index >= 0 && interrupt_index < 256) {
        handlers[interrupt_index] = handler;
    }
}

void isr_common(u64 isr_index, u64 error_code) {
    handlers[isr_index](error_code);
}