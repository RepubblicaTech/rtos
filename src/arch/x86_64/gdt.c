#include <stdint.h>
#include <stdio.h>

#include "gdt.h"

gdt_pointer_t gdtr;

struct __attribute__((packed)) {
    gdt_entry_t gdt_entries[5];
} gdt;

extern void _load_gdt(gdt_pointer_t* descriptor);
extern void _reload_segments(uint64_t cs, uint64_t ds);

// https://wiki.osdev.org/GDT_Tutorial#Flat_/_Long_Mode_Setup
void gdt_init() {
    gdt.gdt_entries[0] = (gdt_entry_t){0, 0, 0, 0, 0, 0};			// Null Segment
	gdt.gdt_entries[1] = (gdt_entry_t){0, 0, 0, 0x9A, 0xA0, 0};		// 64-bit kernel code segment
	gdt.gdt_entries[2] = (gdt_entry_t){0, 0, 0, 0x92, 0xA0, 0};		// 64-bit kernel data segment
	gdt.gdt_entries[3] = (gdt_entry_t){0, 0, 0, 0xFA, 0xA0, 0};		// 64-bit user code segment
	gdt.gdt_entries[4] = (gdt_entry_t){0, 0, 0, 0xF2, 0xA0, 0};		// 64-bit user data segment

    gdtr.size = (uint16_t)(sizeof(gdt) - 1);
    gdtr.pointer = (gdt_entry_t*)&gdt;

	debugf_debug("GDTR:\n");
	debugf_debug("\tsize: %u\n", gdtr.size);
	debugf_debug("\tpointer: %p\n", gdtr.pointer);

	debugf_debug("Loading GDTR %#llx\n", *(uint64_t*)&gdtr);
    _load_gdt(&gdtr);

	_reload_segments(GDT_CODE_SEGMENT, GDT_DATA_SEGMENT);
}
