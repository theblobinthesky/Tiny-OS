#ifdef INTERRUPT_TABLE_C
#define interrupt_no_ec(index, name) \
    void handler_##name(u64); \
    void isr_##name(); \
    set_interrupt_handler((index), handler_##name); \
    set_x64_interrupt_service_routine((index), isr_##name)

#define interrupt_with_ec(index, name) interrupt_no_ec(index, name)

#elif defined(INTERRUPT_TABLE_ASM)
#define interrupt_no_ec(index, name) macro_interrupt_no_ec index name
#define interrupt_with_ec(index, name) macro_interrupt_with_ec index name

#else
#define interrupt_no_ec(index, name)
#define interrupt_with_ec(index, name)
#error "Explicitly define whether the interrupt table is included in c or assembler."
#endif

interrupt_no_ec(0, div_by_zero);
interrupt_no_ec(1, debug);
interrupt_no_ec(2, nmi);
interrupt_no_ec(3, breakpoint);
interrupt_no_ec(4, overflow);
interrupt_no_ec(5, bound_range_exceeded);
interrupt_with_ec(6, invalid_opcode);
interrupt_no_ec(7, device_not_available);
interrupt_with_ec(8, double_fault);
interrupt_with_ec(10, invalid_tss);
interrupt_with_ec(11, segment_not_present);
interrupt_with_ec(12, stack_segment_fault);
interrupt_with_ec(13, general_protection_fault);
interrupt_with_ec(14, page_fault);
interrupt_no_ec(16, x87_fp_exeption);
interrupt_with_ec(17, alignment_check);
interrupt_no_ec(18, machine_check);
interrupt_no_ec(19, simd_fp_exception);
interrupt_no_ec(20, virtualization_exception);
interrupt_no_ec(30, security_exception);

// Add these later...:
// isr_noerror 35
// isr_noerror 36
// isr_noerror 37
// isr_noerror 39

// Also syscall is still missing.

interrupt_no_ec(255, no_ec_default);