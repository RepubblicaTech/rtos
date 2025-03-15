#ifndef SMP_H
#define SMP_H 1

#include <limine.h>

int smp_init();
void mp_trampoline(struct limine_smp_info *cpu);

uint8_t get_cpu();

#endif