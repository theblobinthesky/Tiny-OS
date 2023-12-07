.extern isr_common

.macro save_state
    pushq %rax
    pushq %rbx 
    pushq %rcx 
    pushq %rdx 
    pushq %rsi 
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm

.macro restore_state
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi 
    popq %rdx 
    popq %rcx 
    popq %rbx 
    popq %rax
.endm

.macro macro_interrupt_no_ec num name
.globl isr_\name

isr_\name:
    save_state
    movq $0, %rcx
    movq $\num, %rdx
    call isr_common

    restore_state
    iretq
.endm

.macro macro_interrupt_with_ec num name
.globl isr_\name

isr_\name:
    save_state
    movq 128(%rsp), %rcx
    movq $\num, %rdx
    call isr_common

    restore_state
    iretq
.endm

#define INTERRUPT_TABLE_ASM
#include "interrupt_table.h"
#undef INTERRUPT_TABLE_ASM