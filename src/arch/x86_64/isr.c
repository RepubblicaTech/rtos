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

void print_reg_dump(registers* regs) {
    kprintf("Register dump:\n");

	kprintf("--- GENERAL PURPOSE ---\n");
	kprintf("\trax: %#llx\n"
			"\trbx: %#llx\n"
			"\trcx: %#llx\n"
			"\trdx: %#llx\n",
			regs->rax,
			regs->rbx,
			regs->rcx,
			regs->rdx);
	
	kprintf("--- SEGMENT REGS ---\n");
	kprintf("\tcs (Code segment):   %#llx\n"
			"\tds (Data segment):   %#llx\n"
			"\tss (Stack segment):  %#llx\n",
			regs->cs,
			regs->ds,
			regs->ss);
	
	kprintf("--- FLAGS, POINTER AND INDEX REGISTERS ---\n");
	kprintf("\teflags:%#llx\n"
			"\trip (Instruction address):  %#llx\n"
			"\trbp (Base pointer):         %#llx\n"
			"\trsp (Stack pointer):        %#llx\n"
			"\trdi:                        %#llx\n"
			"\trsi:                        %#llx\n",
			regs->eflags,
			regs->rip,
			regs->rbp,
			regs->rsp,
			regs->rdi,
			regs->rsi);

	kprintf("--- OTHER REGISTERS ---\n");
	kprintf("\tr8:  %#llx\n"
			"\tr9:  %#llx\n"
			"\tr10: %#llx\n"
			"\tr11: %#llx\n"
			"\tr12: %#llx\n"
			"\tr13: %#llx\n"
			"\tr14: %#llx\n"
			"\tr15: %#llx\n",
			regs->r8,
			regs->r9,
			regs->r10,
			regs->r11,
			regs->r12,
			regs->r13,
			regs->r14,
			regs->r15);
}

void isr_handler(registers* regs) {
    if (isr_handlers[regs->interrupt] != NULL) {
        isr_handlers[regs->interrupt](regs);
    } else if (regs->interrupt >= 32) {
        debugf("Unhandled interrupt %d\n", regs->interrupt);
    } else {
        rsod_init();

        kprintf_panic("KERNEL PANIC! \"%s\" (Exception n. %d)\n", exceptions[regs->interrupt], regs->interrupt);

        kprintf("\terrcode: %#llx\n",
                regs->error);
        
        print_reg_dump(regs);


        kprintf("PANIC LOG END --- HALTING ---\n");

        _panic();
    }

}

void isr_registerHandler(int interrupt, isrHandler handler) {
    isr_handlers[interrupt] = handler;
    idt_gate_enable(interrupt);
}
