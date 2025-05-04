#include "ioapic.h"

uint64_t ioapic_base = 0;

void set_ioapic_base(uint64_t base) {
    ioapic_base = base;
}

irq_handler apic_irq_handlers[IOAPIC_REDTBL_ENTRIES];

uint32_t ioapic_reg_read(uint8_t reg) {
    uint32_t volatile *regsel = (uint32_t volatile *)ioapic_base;
    regsel[0]                 = reg;

    return regsel[4];
}

void ioapic_reg_write(uint8_t reg, uint32_t value) {
    uint32_t volatile *regsel = (uint32_t volatile *)ioapic_base;
    regsel[0]                 = reg;

    regsel[4] = value;
}

void ioapic_registerHandler(int irq, irq_handler handler) {
    apic_irq_handlers[irq] = handler;
}