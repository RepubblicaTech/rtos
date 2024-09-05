#include <stdint.h>

#include <gdt.h>

gdt_pointer_t gdtr;

struct __attribute__((packed)) {
    gdt_entry_t gdt_entries[5];
} gdt;

extern void load_gdt(gdt_pointer_t* descriptor);

void reload_segments() {
    __asm__ volatile (
	    "push $0x08\n"
	    "lea 1f(%%rip), %%rax\n"
	    "push %%rax\n"
	    "lretq\n"
	    "1:\n\t"
	    "mov $0x10, %%ax\n\t"
	    "mov %%ax, %%ds\n\t"
	    "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
	    "mov %%ax, %%ss\n"
	    :
        :
	    : "memory"
	);
}

void gdt_init() {
    gdt.gdt_entries[0] = (gdt_entry_t){0, 0, 0, 0, 0, 0};
	gdt.gdt_entries[1] = (gdt_entry_t){0, 0, 0, 0x9A, 0xA0, 0};
	gdt.gdt_entries[2] = (gdt_entry_t){0, 0, 0, 0x92, 0xA0, 0};
	gdt.gdt_entries[3] = (gdt_entry_t){0, 0, 0, 0xFA, 0xA0, 0};
	gdt.gdt_entries[4] = (gdt_entry_t){0, 0, 0, 0xF2, 0xA0, 0};

    gdtr.size = (uint16_t)(sizeof(gdt) - 1);
    gdtr.pointer = (gdt_entry_t*)&gdt;

    load_gdt(&gdtr);

    reload_segments();
}
