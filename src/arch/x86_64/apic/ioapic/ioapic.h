#ifndef IOAPIC_H
#define IOAPIC_H 1

#define IOAPIC_REDTBL_ENTRIES 24

#include <stdint.h>

#include <interrupts/irq.h>

void set_ioapic_base(uint64_t base);

void ioapic_registerHandler(int irq, irq_handler handler);

#endif