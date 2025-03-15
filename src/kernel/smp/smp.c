#include "smp.h"

#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "isr.h"

#include "memory/paging/paging.h"
#include "memory/vmm.h"

#include "mmio/apic/apic.h"

#include "scheduler/scheduler.h"

#include <stdint.h>
#include <stdio.h>

#include <util/util.h>

uint64_t cpu_count = 0;

extern vmm_context_t *kernel_vmm_ctx;

// IPI handler functions
void ipi_test_handler(registers_t *regs) {
    UNUSED(regs);
    uint32_t cpu_id = lapic_get_id();
    debugf_debug("CPU %u received TEST IPI\n", cpu_id);
    lapic_send_eoi();
}

void ipi_reschedule_handler(registers_t *regs) {
    asm("cli");
    // uint32_t cpu_id = lapic_get_id();

    // Trigger scheduling without waiting for next timer tick
    process_handler(regs);

    asm("sti");

    lapic_send_eoi();
}

void ipi_tlb_shoot_handler(registers_t *regs) {
    UNUSED(regs);
    uint32_t cpu_id = lapic_get_id();
    debugf_debug("CPU %u received TLB SHOOTDOWN IPI\n", cpu_id);
    // Invalidate TLB
    asm volatile("invlpg (%0)" ::"r"(0));
    lapic_send_eoi();
}

void ipi_halt_handler(registers_t *regs) {
    UNUSED(regs);
    uint32_t cpu_id = lapic_get_id();
    debugf_warn("CPU %u received HALT IPI - halting\n", cpu_id);
    lapic_send_eoi();
    for (;;) {
        asm("cli; hlt");
    }
}

void register_ipi_handlers() {
    // Register existing handlers
    isr_registerHandler(IPI_VECTOR_TEST, ipi_test_handler);
    isr_registerHandler(IPI_VECTOR_RESCHEDULE, ipi_reschedule_handler);
    isr_registerHandler(IPI_VECTOR_TLB_SHOOT, ipi_tlb_shoot_handler);
    isr_registerHandler(IPI_VECTOR_HALT, ipi_halt_handler);
}

// Send IPI to specific CPU
void send_ipi(uint8_t cpu_id, uint8_t vector) {
    lapic_write_reg(LAPIC_ICR_HIGH, cpu_id << 24);
    lapic_write_reg(LAPIC_ICR_LOW, vector | LAPIC_DELIVERY_MODE_FIXED);
    // Wait for delivery completion
    while (lapic_read_reg(LAPIC_ICR_LOW) & (1 << 12))
        ;
}

// Send IPI to all CPUs including self
void send_ipi_all(uint8_t vector) {
    lapic_write_reg(LAPIC_ICR_HIGH, 0);
    lapic_write_reg(LAPIC_ICR_LOW, vector | LAPIC_DELIVERY_MODE_FIXED |
                                       LAPIC_DEST_ALL_INCLUDING);
    while (lapic_read_reg(LAPIC_ICR_LOW) & (1 << 12))
        ;
}

// Send IPI to all CPUs excluding self
void send_ipi_all_excluding_self(uint8_t vector) {
    lapic_write_reg(LAPIC_ICR_HIGH, 0);
    lapic_write_reg(LAPIC_ICR_LOW, vector | LAPIC_DELIVERY_MODE_FIXED |
                                       LAPIC_DEST_ALL_EXCLUDING);
    while (lapic_read_reg(LAPIC_ICR_LOW) & (1 << 12))
        ;
}

// Send IPI to self
void send_ipi_self(uint8_t vector) {
    lapic_write_reg(LAPIC_ICR_HIGH, 0);
    lapic_write_reg(LAPIC_ICR_LOW,
                    vector | LAPIC_DELIVERY_MODE_FIXED | LAPIC_DEST_SELF);
}

int smp_init(LIMINE_PTR(struct limine_smp_response *) cpus) {
    cpu_count = cpus->cpu_count;

    register_ipi_handlers();

    if (cpus->cpu_count == 1) {
        kprintf_info("SMP init: no other CPUs detected\n");
        return 0;
    }

    kprintf_info("SMP init: %d CPUs detected\n", cpus->cpu_count);
    for (uint64_t i = 0; i < cpus->cpu_count; i++) {
        struct limine_smp_info *cpu = cpus->cpus[i];

        if (cpu->processor_id == 0) {
            // Skip BSP
            continue;
        }

        kprintf_info("Starting CPU %lu...\n", cpu->processor_id);

        cpu->goto_address = mp_trampoline;
    }

    return 0;
}

void mp_trampoline(struct limine_smp_info *cpu) {
    gdt_init();
    idt_init();
    asm("cli");
    isr_init();

    register_ipi_handlers();

    isr_registerHandler(14, pf_handler);

    vmm_switch_ctx(kernel_vmm_ctx);

    apic_init();

    scheduler_init_cpu(cpu->lapic_id);

    asm("sti");

    debugf_debug("CPU %lu initialized and ready. APIC ID: %d\n", cpu->lapic_id,
                 lapic_get_id());

    for (;;)
        send_ipi_self(IPI_VECTOR_RESCHEDULE);
};