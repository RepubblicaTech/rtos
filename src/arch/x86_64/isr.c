#include "isr.h"
#include "gdt.h"
#include "idt.h"
#include "limine.h"
#include "memory/paging/paging.h"
#include "memory/pmm.h"
#include "mmio/apic/apic.h"
#include "smp/ipi.h"
#include "smp/smp.h"
#include <kernel.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

extern struct limine_hhdm_request *hhdm_request;

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
    uint64_t cpu = lapic_get_id();

    if (cpu != 0) {
        debugf_warn(
            "exception \"%s\" (nr. %#llx) on CPU %d @ %.16llx. error code "
            "%#llx. halting...\n",
            exceptions[regs->interrupt], regs->interrupt, cpu, regs->rip,
            regs->error);
        debugf(ANSI_COLOR_RESET);
        asm("cli");
        for (;;)
            _hcf();
    }

    ipi_broadcast(IPI_VECTOR_HALT);

    print_reg_dump(regs);

    // stacktrace
    mprintf("\n\n --- STACK TRACE ---\n");
    uint64_t *rbp = (uint64_t *)regs->rbp;
    int frame     = 0;

    bootloader_data *bootloader_data = get_bootloader_data();

    while (rbp) {
        // In x86_64 calling convention:
        // rbp[0] = previous rbp
        // rbp[1] = return address

        uint64_t return_addr =
            rbp[1]; // Return address (points after call instruction)
        uint64_t *prev_rbp = (uint64_t *)rbp[0];

        // Approximate function address calculation
        // Note: This is a simplification - normally you'd use debug info
        // The function address is typically a few bytes before the return
        // address
        uint64_t approx_func_addr =
            return_addr - 5; // Typical x86_64 call instruction is 5 bytes

        // If this is in the higher-half kernel space, convert to physical
        // address
        uint64_t phys_addr = 0;
        if (return_addr >= 0xffff800000000000) {
            // Using HHDM offset from limine bootloader
            phys_addr = return_addr - bootloader_data->hhdm_offset;
            mprintf("Frame %d: [%p] func addr: %#llx (phys: %#llx)\n", frame,
                    rbp, approx_func_addr, phys_addr);
        } else {
            mprintf("Frame %d: [%p] func addr: %#llx\n", frame, rbp,
                    approx_func_addr);
        }

        // Move to previous frame
        rbp = prev_rbp;
        frame++;

        // Guard against invalid pointers or corrupted stack
        if (frame > 20 || !rbp || (uint64_t)rbp < 0x1000)
            break;
    }
    mprintf("\nPANIC LOG END --- HALTING ---\n");
    debugf(ANSI_COLOR_RESET);
    asm("cli");
    for (;;)
        _hcf();
}

void isr_handler(registers_t *regs) {

    if (isr_handlers[regs->interrupt] != NULL) {
        isr_handlers[regs->interrupt](regs);
    } else if (regs->interrupt >= 32) {
        debugf_warn("Unhandled interrupt %d on CPU %d\n", regs->interrupt,
                    lapic_get_id());
    } else {
        uint64_t cpu = lapic_get_id();
        if (cpu == 0) {
            stdio_panic_init();

            fb_set_fg(PANIC_FG);
            mprintf("KERNEL PANIC! \"%s\" (Exception n. %d)\n",
                    exceptions[regs->interrupt], regs->interrupt);

            mprintf("\terrcode: %#llx\n", regs->error);
        }

        panic_common(regs);

        _hcf();
    }
}

void isr_registerHandler(int interrupt, isrHandler handler) {
    isr_handlers[interrupt] = handler;
    idt_gate_enable(interrupt);
}
