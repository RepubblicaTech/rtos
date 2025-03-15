#include <smp/smp.h>

#include <stdint.h>
#include <stdio.h>

#include <gdt.h>
#include <idt.h>
#include <isr.h>
#include <memory/paging/paging.h>
#include <memory/vmm.h>
#include <mmio/apic/apic.h>
#include <smp/ipi.h>
#include <scheduler/scheduler.h>
#include <kernel.h>

#include <util/util.h>

extern vmm_context_t *kernel_vmm_ctx;

int smp_init() {

    bootloader_data *bootloader_data = get_bootloader_data();

    register_ipi();

    if (bootloader_data->cpu_count == 1) {
        kprintf_info("SMP init: no other CPUs detected\n");
        return 0;
    }

    kprintf_info("SMP init: %d CPUs detected\n", bootloader_data->cpu_count);
    for (uint64_t i = 0; i < bootloader_data->cpu_count; i++) {
        struct limine_smp_info *cpu = bootloader_data->cpus[i];

        if (cpu->processor_id == 0) {
            // dont init the bsp (limine shouldnt give it but just in case)
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

    vmm_switch_ctx(kernel_vmm_ctx);

    apic_init();

    register_ipi();

    debugf_ok("CPU %lu initialized and ready. APIC ID: %d\n", cpu->lapic_id,
              lapic_get_id());


    asm("sti");

    for (;;)
        ;
}

uint8_t get_cpu() {
    return lapic_get_id();
}