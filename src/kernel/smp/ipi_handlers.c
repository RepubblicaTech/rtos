#include "memory/vmm.h"
#include <smp/ipi.h>

#include <stdio.h>

#include <isr.h>
#include <mmio/apic/apic.h>
#include <scheduler/scheduler.h>
#include <smp/smp.h>

void do_nothing_and_shut_up_im_talking_to_you_vector_254_yes_you_just_dont_spam_logs_ok_thanks(
    registers_t *regs) {
    (void)regs;
    lapic_send_eoi();
}

void ipi_handler_halt(registers_t *regs) {
    // beauty only 💅
    uint64_t cpu = get_cpu();
    debugf_warn("Processor %lu halted over IPI @ %.16llx\n", cpu, regs->rip);
    lapic_send_eoi();

    // actual halting
    asm("cli");
    for (;;)
        asm("hlt");
}

void ipi_handler_tlb_flush(registers_t *regs) {
    uint64_t cpu = get_cpu();

    debugf_debug("Processor %lu flushed TLB @ %#llx\n", cpu, regs->rip);

    asm("mov %0, %%cr3" : : "r"(get_current_ctx()->pml4_table));
    lapic_send_eoi();
}

void ipi_handler_reschedule(registers_t *regs) {
    uint64_t cpu = get_cpu();
    debugf_debug("Processor %lu rescheduled @ %.16llx\n", cpu, regs->rip);
    lapic_send_eoi();
    scheduler_schedule(regs);
}

void ipi_handler_test(registers_t *regs) {
    uint64_t cpu = get_cpu();
    debugf_debug("Processor %lu received test IPI @ %.16llx\n", cpu, regs->rip);
    lapic_send_eoi();
}
