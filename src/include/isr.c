#include <isr.h>
#include <idt.h>
#include <gdt.h>
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

extern void panic();

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
        printf("\n-----------------------------------------------------------------\n");
        printf("PANIC! --- \"%s\" (Exception n. %d)\n", exceptions[regs->interrupt], regs->interrupt);
        printf("  rax=0x%x  rbx=0x%x  rcx=0x%x  rdx=0x%x  rsi=0x%x  rdi=0x%x\n",
               regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi);
        printf("  rsp=0x%x  rbp=0x%x  rip=0x%x  eflags=0x%x  cs=0x%x  ds=0x%x  ss=0x%x\n",
               regs->rsp, regs->rbp, regs->rip, regs->eflags, regs->cs, regs->ds, regs->ss);
        printf("  interrupt=0x%x  errorcode=0x%x\n", regs->interrupt, regs->error);
        printf("-----------------------------------------------------------------\n");

        panic();
    }
    
}

void isr_registerHandler(int interrupt, isrHandler handler) {
    isr_handlers[interrupt] = handler;
    idt_gate_enable(interrupt);
}
