#include "cpu.h"

bool check_pae() {
    uint64_t cr4 = cpu_get_cr(4);

    return cr4 & (1 << 5);
}

bool check_msr() {
    static uint32_t eax, unused, edx;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_EDX_MSR;
}

bool check_apic() {
    uint32_t eax, edx, unused;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_EDX_APIC;
}

bool check_tsc() {
    uint32_t edx, unused;
    __get_cpuid(0x01, &unused, &unused, &unused, &edx);
    return (edx & (1 << 4)) ? 1 : 0;
}

bool check_x2apic() {
    uint32_t eax, edx, unused;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_ECX_X2APIC;
}

bool check_fpu() {
    uint32_t eax, edx, unused;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_EDX_FPU;
}

bool check_sse() {
    uint32_t eax, edx, unused;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_EDX_SSE;
}

bool check_sse2() {
    uint32_t eax, edx, unused;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_EDX_SSE2;
}

bool check_fxsr() {
    uint32_t eax, edx, unused;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_EDX_FXSR;
}

void cpu_reg_write(uint32_t *reg, uint32_t value) {
    *reg = value;
}

uint32_t cpu_reg_read(uint32_t *reg) {
    return *reg;
}

const char *get_cpu_vendor() {
    unsigned int eax, ebx, ecx, edx;
    static char vendor[13];
    if (__get_cpuid(0, &eax, &ebx, &ecx, &edx)) {
        ((unsigned int *)vendor)[0] = ebx;
        ((unsigned int *)vendor)[1] = edx;
        ((unsigned int *)vendor)[2] = ecx;
        vendor[12]                  = '\0';
        return vendor;
    } else {
        return "UNKNOWN";
    }
}
