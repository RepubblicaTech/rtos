#include "cpu.h"

/* Get CPU's model number */
static int get_model(void) {
    int ebx, unused;
    __cpuid(0, unused, ebx, unused, unused);
    return ebx;
}

bool check_pae() {
   uint64_t cr4 = cpu_get_cr(4);

   return cr4 & (1 << 5);
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
   uint32_t eax = lo;
   uint32_t edx = hi;
   asm volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(msr));
}

bool check_apic() {
   uint32_t eax, edx, unused;
   __get_cpuid(1, &eax, &unused, &unused, &edx);
   return edx & CPUID_FEAT_EDX_APIC;
}
