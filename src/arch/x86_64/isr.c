#include "isr.h"
#include "gdt.h"
#include "idt.h"

#include <stddef.h>
#include <stdio.h>

isrHandler isr_handlers[IDT_MAX_DESCRIPTORS];

static const char *const exceptions[] = {"Divide by zero error",
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
                                         ""};

void isr_init() {
    for (int i = 0; i < 256; i++) {
        idt_gate_enable(i);
    }

    idt_gate_disable(0x80);
}

void print_reg_dump(registers_t *regs) {
    debugf(ANSI_COLOR_RED);

    mprintf("\nRegister dump:\n\n");

    mprintf("--- GENERAL PURPOSE REGISTERS ---\n");
    mprintf("rax: 0x%.016llx r8: 0x%.16llx\n"
            "rbx: 0x%.016llx r9: 0x%.16llx\n"
            "rcx: 0x%.016llx r10: 0x%.16llx\n"
            "rdx: 0x%.016llx r11: 0x%.16llx\n"
            "\t\t\tr12: 0x%.016llx\n"
            "\t\t\tr13: 0x%.016llx\n"
            "\t\t\tr14: 0x%.016llx\n"
            "\t\t\tr15: 0x%.016llx\n",
            regs->rax, regs->r8, regs->rbx, regs->r9, regs->rcx, regs->r10,
            regs->rdx, regs->r11, regs->r12, regs->r13, regs->r14, regs->r15);

    mprintf("\n--- SEGMENT REGS ---\n");
    mprintf("\tcs (Code segment):   %#llx\n"
            "\tds (Data segment):   %#llx\n"
            "\tss (Stack segment):  %#llx\n",
            regs->cs, regs->ds, regs->ss);

    mprintf("\n--- FLAGS, POINTER AND INDEX REGISTERS ---\n");
    mprintf("\teflags:%#llx\n"
            "\trip (Instruction address):  %#llx\n"
            "\trbp (Base pointer):         %#llx\n"
            "\trsp (Stack pointer):        %#llx\n"
            "\trdi:                        %#llx\n"
            "\trsi:                        %#llx\n",
            regs->rflags, regs->rip, regs->rbp, regs->rsp, regs->rdi,
            regs->rsi);
}

void panic_common(registers_t *regs) {
    print_reg_dump(regs);

    // stacktrace
    mprintf("\n\n --- STACK TRACE ---\n");
    // ignore the 128 bytes red zone
    for (uint64_t *sp = (uint64_t *)(regs->rsp); sp <= (uint64_t *)regs->rbp;
         sp++) {
        mprintf("%p: %#llx\n", sp, *sp);
    }

    mprintf("\nPANIC LOG END --- HALTING ---\n");
    debugf(ANSI_COLOR_RESET);
    _hcf();
}

void isr_handler(registers_t *regs) {

    if (isr_handlers[regs->interrupt] != NULL) {
        isr_handlers[regs->interrupt](regs);
    } else if (regs->interrupt >= 32) {
        debugf("Unhandled interrupt %d\n", regs->interrupt);
    } else {
        stdio_panic_init();

        fb_set_fg(PANIC_FG);
        mprintf("KERNEL PANIC! \"%s\" (Exception n. %d)\n",
                exceptions[regs->interrupt], regs->interrupt);

        mprintf("\terrcode: %#llx\n", regs->error);

        panic_common(regs);

        _hcf();
    }
}

void isr_registerHandler(int interrupt, isrHandler handler) {
    isr_handlers[interrupt] = handler;
    idt_gate_enable(interrupt);
}
