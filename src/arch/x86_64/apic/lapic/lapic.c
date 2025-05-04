#include "lapic.h"

#include <interrupts/irq.h>

#include <cpu.h>

bool lapic_status = false;

uint64_t lapic_base = 0;

void set_lapic_base(uint64_t base) {
    lapic_base = base;
}

bool is_lapic_enabled() {
    return lapic_status;
}

void lapic_write_reg(uint64_t reg, uint32_t value) {
    if (!lapic_base)
        return;

    cpu_reg_write((uint32_t *)(lapic_base + reg), value);
}

uint64_t lapic_read_reg(uint64_t reg) {
    if (!lapic_base)
        return 0;

    return cpu_reg_read((uint32_t *)(lapic_base + reg));
}

uint64_t lapic_get_id() {
    return lapic_read_reg(LAPIC_ID_REG);
}

void lapic_timer_init() {
}

void lapic_send_eoi() {
    lapic_write_reg(LAPIC_EOI_REG, 0);
}

void lapic_init() {
}
