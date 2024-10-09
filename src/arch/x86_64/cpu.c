#include "cpu.h"

#include <cpuid.h>
#include "pic.h"

/* Get CPU's model number */
static int get_model(void) {
    int ebx, unused;
    __cpuid(0, unused, ebx, unused, unused);
    return ebx;
}

bool check_msr() {
   static uint32_t a, unused, d; // eax, edx
   __get_cpuid(1, &a, &unused, &unused, &d);
   return d & CPUID_FEAT_EDX_MSR;
}

void cpu_get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi) {
   asm volatile("rdtsc" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void cpu_set_msr(uint32_t msr, uint32_t lo, uint32_t hi) {
   asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}
