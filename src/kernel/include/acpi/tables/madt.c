#include "madt.h"

#include <acpi/acpi.h>

#include <util/string.h>

#include <mmio/mmio.h>

#include <stddef.h>
#include <stdio.h>

#include <kernel.h>

#include <cpu.h>

int ioapic_int_overrides = 0;

madt* madt_h;

madt* get_madt() {
	return madt_h;
}

// find a MADT record entry given the entry type
madt_record* find_record(madt* madt_h, const uint8_t entry_type) {
	for (size_t offset = 0x2c; offset < madt_h->header.length;)
	{
		madt_record* record = (madt_record*)((uint8_t*)madt_h + offset);
		if (record->entry_type == entry_type) {
			// we found the record entry!
			return record;
		}

		offset += record->entry_length;
	}
	// we didn't find anything
	return NULL;
}

// we got three ways to check if APIC is supported:
// 1. MSR 0x1B
// 2. MADT header
// 3. MADT LAPIC

void madt_devices_init() {
	uint64_t* root_sdt = get_root_sdt();
	madt_h = (madt*)find_table((acpi_sdt_header*)((uint64_t*)PHYS_TO_VIRTUAL(root_sdt)), "APIC");

	madt_lapic* lapic = (madt_lapic*)find_record(madt_h, MADT_ENTRY_LAPIC);
	kprintf_info("LAPIC ID: %#hhx\n", lapic->id);
	kprintf_info("LAPIC flags: %#02b\n", lapic->flags);

	madt_lapic_addr_ovveride* lapic_override = (madt_lapic_addr_ovveride*)find_record(madt_h, MADT_ENTRY_LAPIC_ADDR_OVVERRIDE);

	uint64_t lapic_base = (lapic_override == 0x0
						   ? ((uint64_t)madt_h->lapic_control_address == 0x0
						   	  ? _cpu_get_msr(0x1B) & 0xfffff000
							  : (uint64_t)madt_h->lapic_control_address)
						   : lapic_override->physical_base);

	kprintf_info("LAPIC base address: (phys)%#llx\n", lapic_base);

	mmio_device mmio_lapic = {
		.base = (uint64_t)lapic_base,
		.size = 0x1000,
		.name = "LAPIC"
	};
	append_mmio(mmio_lapic);

	madt_ioapic* io_apic = (madt_ioapic*)find_record(madt_h, MADT_ENTRY_IOAPIC);
	kprintf_info("I/O APIC base address: (phys)%#lx\n", io_apic->address);
	kprintf_info("I/O APIC global interrupt base: %#lx\n", io_apic->global_sys_interrupt_base);

	mmio_device mm_io_apic = {
		.base = (uint64_t)io_apic->address,
		.size = 0x1000,
		.name = "IO_APIC"
	};
	append_mmio(mm_io_apic);
}
