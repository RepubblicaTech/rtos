#ifndef IO_APIC_H
#define IO_APIC_H 1

#define IOAPIC_IRQ_OFFSET	0x20

#define MMIO_APIC_SIG		"IO_APIC"

void ioapic_init();

#endif