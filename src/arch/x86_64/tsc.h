#ifndef TSC_H
#define TSC_H 1

#include <isr.h>
#include <stdint.h>

extern uint64_t _get_tsc();
void tsc_tick_handler(registers_t *regs);
void tsc_sleep(uint64_t microseconds);
uint64_t get_cpu_freq_msr();

void tsc_init();

#endif
