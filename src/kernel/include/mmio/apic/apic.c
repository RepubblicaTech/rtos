#include "apic.h"

#include <acpi/tables/madt.h>

#include <cpu.h>
#include <pic.h>
#include <pit.h>

#include <kernel.h>

#include <stdio.h>

#include <util/string.h>
#include <io/io.h>

#include <mmio/mmio.h>

static struct bootloader_data limine_data;

// write data to a LAPIC register
// by https://github.com/Tix3Dev/apoptOS/blob/370fd34a6d3c87a9d1a16d1a2ec072bd1836ba6c/src/kernel/hardware/apic/apic.c
void lapic_write_reg(uint32_t reg, uint32_t data)
{
	cpu_reg_write(limine_data.p_lapic_base + reg, data);
}

// get data from a LAPIC register
// https://github.com/Tix3Dev/apoptOS/blob/370fd34a6d3c87a9d1a16d1a2ec072bd1836ba6c/src/kernel/hardware/apic/apic.c
uint32_t lapic_read_reg(uint32_t reg)
{
    return cpu_reg_read(limine_data.p_lapic_base + reg);
}

void lapic_set_base(uint64_t lapic_base) {
    _cpu_set_msr(0x1B, lapic_base);
}

uint8_t lapic_get_id() {
	return (uint8_t)lapic_read_reg(LAPIC_ID_REG);
}

void lapic_send_eoi() {
	lapic_write_reg(LAPIC_EOI_REG, 0);
}

uint64_t apic_get_base() {
	return (_cpu_get_msr(0x1b) & 0xfffff000);
}

void apic_init() {
	limine_data = get_bootloader_data();

	mmio_device mm_lapic = find_mmio(MMIO_LAPIC_SIG);
	debugf_debug("MMIO device \"%s\" base:%#llx size:%#llx\n", mm_lapic.name, mm_lapic.base, mm_lapic.size);
	limine_data.p_lapic_base = PHYS_TO_VIRTUAL(mm_lapic.base);
	kprintf_info("LAPIC base address: %#llx\n", limine_data.p_lapic_base);

	// set the actual APIC base
	kprintf_info("APIC_BASE_MSR: %#llx; LAPIC actual base: %#llx\n", _cpu_get_msr(0x1b), apic_get_base());
	uint64_t apic_msr_new = (apic_get_base() & 0x0ffffff0000) | 0x100;
	kprintf_info("Setting APIC MSR to %#llx\n", apic_msr_new);
	_cpu_set_msr(0x1b, apic_msr_new);

	// disable the PIC
	pic_disable();
	// check apic.asm for more
	_apic_global_enable();
	lapic_write_reg(LAPIC_TASKPR_REG, 0);
	lapic_write_reg(LAPIC_DEST_FMT_REG, 0xFFFFFFFF);
	kprintf_info("LAPIC is globally enabled and MSR is now %#llx\n", _cpu_get_msr(0x1b));
	lapic_write_reg(LAPIC_SPURIOUS_REG, 0x1ff);
	kprintf_info("LAPIC_SPURIOUS_REG: %#lx\n", lapic_read_reg(LAPIC_SPURIOUS_REG));

	lapic_write_reg(LAPIC_LINT0_REG, 0xfe);
	lapic_write_reg(LAPIC_LINT1_REG, 0xfe);
	lapic_write_reg(LAPIC_CMCI_REG, 0xfe);
	lapic_write_reg(LAPIC_ERR_REG, 0xfe);
	lapic_write_reg(LAPIC_PERFMON_COUNT_REG, 0xfe);
	lapic_write_reg(LAPIC_THERM_REG, 0xfe);
	lapic_write_reg(LAPIC_TIMER_REG, 0xfe);

	kprintf_info("LAPIC ID: %#hhx\n", lapic_get_id());
}
