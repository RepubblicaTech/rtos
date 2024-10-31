#include "isr.h"
#include "idt.h"
#include "gdt.h"
#include <stdio.h>
#include <stddef.h>

isrHandler isr_handlers[IDT_MAX_DESCRIPTORS];

static const char* const exceptions[] = {
    "Divide by zero error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception ",
    "",
    "",
    "",
    "",
    "",
    "",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    ""
};

void isr_init() {
    for (int i = 0; i < 256; i++) {
        idt_gate_enable(i);
    }

    idt_gate_disable(0x80);
    
}

void isr_handler(registers* regs) {
    if (isr_handlers[regs->interrupt] != NULL) {
        isr_handlers[regs->interrupt](regs);
    } else if (regs->interrupt >= 32) {
        debugf("Unhandled interrupt %d\n", regs->interrupt);
    }
    else {
        kprintf("\n-----------------------------------------------------------------\n");
        kprintf("PANIC! --- \"%s\" (Exception n. %d)\n", exceptions[regs->interrupt], regs->interrupt);
        kprintf("  rax=0x%X  rbx=0x%X  rcx=0x%X  rdx=0x%X  rsi=0x%X  rdi=0x%X\n",
               regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi);
        kprintf("  rsp=0x%X  rbp=0x%X  rip=0x%X  eflags=0x%X  cs=0x%X  ds=0x%X  ss=0x%X\n",
               regs->rsp, regs->rbp, regs->rip, regs->eflags, regs->cs, regs->ds, regs->ss);
        kprintf("  interrupt=0x%X  errorcode=0x%X\n", regs->interrupt, regs->error);
        kprintf("-----------------------------------------------------------------\n");

        panic();
    }

}

void isr_registerHandler(int interrupt, isrHandler handler) {
    isr_handlers[interrupt] = handler;
    idt_gate_enable(interrupt);
}
