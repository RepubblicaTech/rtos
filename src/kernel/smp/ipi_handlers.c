#include "kernel.h"
#include <smp/ipi.h>

#include <stdio.h>

#include <isr.h>
#include <mmio/apic/apic.h>
#include <smp/smp.h>

void do_nothing_and_shut_up_im_talking_to_you_vector_254_yes_you_just_dont_spam_logs_ok_thanks(registers_t *regs) {
    (void)regs;
    lapic_send_eoi();
}

void ipi_handler_halt(registers_t *regs) {
    // beauty only 💅
    uint64_t cpu = get_cpu();
    debugf_warn("Processor %lu halted over IPI @ %.16llx\n", cpu, regs->rip);
    lapic_send_eoi();


    // actual halting
    asm ("cli");
    for (;;)
        asm("hlt");
}

void ipi_handler_tlb_flush(registers_t *regs) {
    uint64_t cpu = get_cpu();
    debugf_debug("Processor %lu TLB flushed @ %.16llx\n", cpu, regs->rip);
    lapic_send_eoi();

    asm volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" : : : "rax");
        // ! idk how to pass only a certain address so we flush the whole thing
}

void ipi_handler_reschedule(registers_t *regs) {
    uint64_t cpu = get_cpu();
    debugf_debug("Processor %lu rescheduled @ %.16llx\n", cpu, regs->rip);
    lapic_send_eoi();
    // todo reschedule
}

void ipi_handler_test(registers_t *regs) {
    uint64_t cpu = get_cpu();
    debugf_debug("Processor %lu received test IPI @ %.16llx\n", cpu, regs->rip);
    lapic_send_eoi();
}
