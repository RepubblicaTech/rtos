#ifndef IO_APIC_H
#define IO_APIC_H 1

#include <idt.h>
#include <irq.h>

#define IOAPIC_IRQ_OFFSET 0x20
#define IOREDTBL_ENTRIES  24

#define MMIO_APIC_SIG "IO_APIC"

uint64_t *get_redtbl();

uint32_t ioapic_reg_read(uint8_t reg);
void ioapic_reg_write(uint8_t reg, uint32_t value);

void apic_registerHandler(int irq, irq_handler handler);

void ioapic_map_irq(int irq, int interrupt, irq_handler handler);

void ioapic_init();
void apic_unregisterHandler(int irq);

#endif