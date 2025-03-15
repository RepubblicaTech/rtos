#ifndef APIC_H
#define APIC_H 1

#include <isr.h>
#include <stdint.h>

// LAPIC memory-mapped registers
// from
// https://github.com/Tix3Dev/apoptOS/blob/370fd34a6d3c87a9d1a16d1a2ec072bd1836ba6c/src/kernel/hardware/apic/apic.h

#define LAPIC_ID_REG         0x020
#define LAPIC_VER_REG        0x030
#define LAPIC_TASKPR_REG     0x080
#define LAPIC_EOI_REG        0x0B0
#define LAPIC_LOCAL_DEST_REG 0x0D0
#define LAPIC_DEST_FMT_REG   0x0E0
#define LAPIC_SPURIOUS_REG   0x0F0

#define LAPIC_INSERVICE_REG 0x100
#define LAPIC_INT_REQ_REG   0x200

#define LAPIC_CMCI_REG 0x2f0

#define LAPIC_ICR0_REG 0x300
#define LAPIC_ICR1_REG 0x310

#define LAPIC_TIMER_REG         0x320
#define LAPIC_THERM_REG         0x330
#define LAPIC_PERFMON_COUNT_REG 0x340
#define LAPIC_LINT0_REG         0x350
#define LAPIC_LINT1_REG         0x360
#define LAPIC_ERR_REG           0x370

#define LAPIC_TIMER_INIT_CNT 0x380
#define LAPIC_TIMER_CURR_CNT 0x390
#define LAPIC_TIMER_DIV_REG  0x3E0

#define LAPIC_ENABLE_BIT (1 << 8)

#define LAPIC_IRQ_OFFSET 239

#define LAPIC_TIMER_VECTOR    0
#define LAPIC_SPURIOUS_VECTOR 7

#define LAPIC_DISABLE (1 << 16)
#define LAPIC_NMI     (4 << 8)

// LDT: Lapic Destination Type
#define LDT_NO_SHORTHAND      0b00
#define LDT_SEND_TO_US        0b01
#define LDT_ALL_LAPICS        0b10
#define LDT_ALL_LAPICS_NOT_US 0b11

#define MMIO_LAPIC_SIG "LAPIC"

void lapic_write_reg(uint32_t reg, uint32_t data);
uint32_t lapic_read_reg(uint32_t reg);

uint8_t lapic_get_id();
void lapic_send_eoi();

uint64_t apic_get_base();

void apic_init();

extern uint64_t _apic_global_enable();

uint32_t lapic_timer_calibrate(void);
void lapic_timer_init(void);
void lapic_timer_handler(registers_t *regs);
extern uint32_t lapic_timer_ticks_per_ms;

#endif
