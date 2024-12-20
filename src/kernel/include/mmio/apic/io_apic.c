#include "io_apic.h"
#include "apic.h"

#include <kernel.h>

#include <stdio.h>

#include <util/string.h>

#include <acpi/tables/madt.h>
#include <mmio/mmio.h>

#include <pit.h>

mmio_device* mmios;
mmio_device mm_io_apic;
madt_ioapic* io_apic;

static struct bootloader_data limine_data;

uint64_t* ioapic_redtbl;
uint64_t* get_redtbl() {
	return ioapic_redtbl;
}

uint32_t ioapic_reg_read(uint8_t reg) {
	uint32_t volatile * regsel = (uint32_t volatile *)limine_data.p_ioapic_base;
	regsel[0] = reg;

	return regsel[4];
}

void ioapic_reg_write(uint8_t reg, uint32_t value) {
	uint32_t volatile * regsel = (uint32_t volatile *)limine_data.p_ioapic_base;
	regsel[0] = reg;

	regsel[4] = value;
}

irq_handler apic_irq_handlers[23];

void apic_irq_handler(registers* regs) {
	int apic_irq = regs->interrupt - IOAPIC_IRQ_OFFSET;

	uint64_t apic_isr = lapic_read_reg(LAPIC_INSERVICE_REG);
	uint64_t apic_irr = lapic_read_reg(LAPIC_INT_REQ_REG);

	if (apic_irq_handlers[apic_irq] != NULL) {
		apic_irq_handlers[apic_irq](regs);
	} else {
		debugf("Unhandled IRQ %d  ISR=%#llx  IRR=%#llx\n", apic_irq, apic_isr, apic_irr);
	}

	lapic_send_eoi();
}

void apic_register_handler(int irq, irq_handler handler) {
	apic_irq_handlers[irq] = handler;
}

// maps an I/O APIC IRQ to an interrupt that calls the handler if fired
// @param irq			- The hardware interrupt that'll be fired
// @param interrupt		- This needs to be an interrupt vector that will be written to the I/O APIC
// @param handler		- The ISR that will be called when the interrupt gets fired
void ioapic_map_irq(int irq, int interrupt, irq_handler handler) {
	uint64_t redtble_data = (lapic_get_id() << 56) |
							(0 << 16)			   |	// Unmask the entry
							(0 << 11)			   |	// Destination mode
							(0b000 << 8)		   |	// Delivery mode
							interrupt;					// interrupt vector
	ioapic_reg_write(0x10 + (2 * irq), (uint32_t)(redtble_data & 0xFFFF));
	ioapic_reg_write(0x10 + (2 * irq) + 1, (uint32_t)((redtble_data >> 32) & 0xFFFF));

	isr_registerHandler(interrupt, handler);
}

void ioapic_init() {
	limine_data = get_bootloader_data();

	mm_io_apic = find_mmio(MMIO_APIC_SIG);
	limine_data.p_ioapic_base = mm_io_apic.base;
	kprintf_info("APIC Base address: %#lx\n", limine_data.p_ioapic_base);
	
	uint8_t ioapic_redir_entries = (ioapic_reg_read(0x01) >> 16) & 0xFF;
	kprintf_info("Redirection entries: %#hhu\n", ioapic_redir_entries);

	uint64_t ioredtbl[ioapic_redir_entries];
	int redir = 0;
	for (int i = 0x10; i < 0x3f; i+=2)
	{
		ioredtbl[redir] = ioapic_reg_read(i) | (ioapic_reg_read(i + 1) << 32);
		debugf("I/O APIC redirection entry %d: %#08llx\n", redir, ioredtbl[redir]);
		redir++;
	}

	// now that we got the redirection entries, we should now get all IRQ overrides
	madt_ioapic_int_override* ioapic_int_override[ioapic_redir_entries];
	int iso_entry = 0;
	madt* madt_h = get_madt();
	// 18/12/2024: Let' read and save all redirection entries :)
	for (size_t offset = 0x2c; offset < madt_h->header.length;)
	{
		madt_record* record = (madt_record*)((uint8_t*)madt_h + offset);
		if (record->entry_type == MADT_ENTRY_IOAPIC_INT_OVERRIDE) {
			ioapic_int_override[iso_entry] = (madt_ioapic_int_override*)record;
			iso_entry++;
		}
		offset += record->entry_length;
	}
	ioapic_redtbl = ioredtbl;
	int overrideable_ints[iso_entry];
	for (int i = 0; i < iso_entry; i++)
	{
		overrideable_ints[i] = ioapic_int_override[i]->legacy_irq_source;
	}
	
	for (int i = 0; i < iso_entry; i++)
	{
		debugf("Interrupt override n.%d\n", i);
		debugf("\tI/O APIC IRQ: %lu\n", ioapic_int_override[i]->ioapic_irq);
		debugf("\tLegacy PIC IRQ: %hhu\n", ioapic_int_override[i]->legacy_irq_source);

		ioapic_map_irq(ioapic_int_override[i]->ioapic_irq,
					   IOAPIC_IRQ_OFFSET + ioapic_int_override[i]->legacy_irq_source, 
					   apic_irq_handler);
	}
	for (int i = 0; i < ioapic_redir_entries; i++)
	{
		// to map the other normal interrupts, we'll just check if it's masked or not
		if (ioapic_reg_read(0x10 + (2 * i)) & (1 << 16)) {
			ioapic_map_irq(i, i + IOAPIC_IRQ_OFFSET, apic_irq_handler);
		}
	}
	apic_register_handler(0, pit_tick);
	
}
