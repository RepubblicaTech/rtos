#include "fpu.h"

#include <stdio.h>

bool fpu_enabled = false;

void init_fpu() {
    if (!check_fpu()) {
        kprintf_panic("x87 FPU not available!\n");
        _hcf();
    }

    asm volatile(
        "mov %%cr0, %%rax\n"
        "and $~(1 << 2), %%rax\n" // clear EM: disable emulation
        "or  $(1 << 1), %%rax\n"  // set MP: monitor FPU
        "or  $(1 << 5), %%rax\n"  // set NE: enable internal x87 error reporting
        "mov %%rax, %%cr0\n" ::
            : "rax");

    asm volatile("fninit");

    fpu_enabled = true;
}