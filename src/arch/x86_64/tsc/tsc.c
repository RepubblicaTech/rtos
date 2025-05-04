#include "tsc.h"
#include "kernel.h"
#include "scheduler/scheduler.h"

#include <interrupts/isr.h>
#include <stdio.h>
#include <time.h>
#include <util/util.h>

volatile uint64_t tsc_ticks = 0;

uint64_t cpu_frequency_hz1 = 0;

void tsc_tick_handler(void *ctx) {
    registers_t *regs = ctx;
    UNUSED(regs);

    static uint64_t last_tsc = 0;
    uint64_t current_tsc     = _get_tsc();

    uint64_t elapsed_tsc = current_tsc - last_tsc;

    uint64_t elapsed_ms = (elapsed_tsc * 1000) / cpu_frequency_hz1;

    if (elapsed_ms >= 100) {
        tsc_ticks++;
        set_ticks(get_ticks() + 1);
        last_tsc = current_tsc;
    }
}

void tsc_sleep(uint64_t microseconds) {
    asm volatile("cli");
    uint64_t start = _get_tsc();

    uint64_t cycles_to_wait = (microseconds * cpu_frequency_hz1) / 1000000;

    while ((_get_tsc() - start) < cycles_to_wait)
        ;
    asm volatile("sti");
}

uint64_t get_cpu_freq_msr() {
    uint64_t start_tsc, end_tsc;

    start_tsc = _get_tsc();

    tsc_sleep(1000000000);

    end_tsc = _get_tsc();

    uint64_t tsc_diff = end_tsc - start_tsc;

    uint64_t cpu_frequency = (uint64_t)tsc_diff;

    return cpu_frequency;
}

void tsc_init() {
    cpu_frequency_hz1 = get_cpu_freq_msr();

    isr_registerHandler(238, tsc_tick_handler);

    debugf_debug("Successfully initialized TSC with CPU Frequency %d Hz\n",
                 cpu_frequency_hz1);
}
