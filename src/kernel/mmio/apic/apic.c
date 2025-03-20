#include "time.h"
#include <mmio/apic/apic.h>

#include <acpi/tables/madt.h>

#include <cpu.h>
#include <pic.h>
#include <pit.h>
#include <tsc.h>

#include <kernel.h>

#include <stdio.h>

#include <io.h>
#include <util/string.h>
#include <util/util.h>

#include <mmio/mmio.h>

#include <memory/pmm.h>

static struct bootloader_data *limine_data;

extern volatile int tsc;

uint32_t lapic_timer_ticks_per_ms = 0;

// write data to a LAPIC register
// by
// https://github.com/Tix3Dev/apoptOS/blob/370fd34a6d3c87a9d1a16d1a2ec072bd1836ba6c/src/kernel/hardware/apic/apic.c
void lapic_write_reg(uint32_t reg, uint32_t data) {
    cpu_reg_write((uint32_t *)PHYS_TO_VIRTUAL(limine_data->p_lapic_base + reg),
                  data);
}

// get data from a LAPIC register
// https://github.com/Tix3Dev/apoptOS/blob/370fd34a6d3c87a9d1a16d1a2ec072bd1836ba6c/src/kernel/hardware/apic/apic.c
uint32_t lapic_read_reg(uint32_t reg) {
    return cpu_reg_read(
        (uint32_t *)PHYS_TO_VIRTUAL(limine_data->p_lapic_base + reg));
}

void lapic_set_base(uint64_t lapic_base) {
    _cpu_set_msr(0x1B, lapic_base);
}

uint8_t lapic_get_id() {
    return (uint8_t)(lapic_read_reg(LAPIC_ID_REG) >> 24) & 0xFF;
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
    debugf_debug("MMIO device \"%s\" base:%#llx size:%#llx\n", mm_lapic.name,
                 mm_lapic.base, mm_lapic.size);
    limine_data->p_lapic_base = mm_lapic.base;
    debugf_debug("LAPIC base address: %#llx\n", mm_lapic.base);

    // disable the PIC
    pic_disable();
    // check apic.asm for more
    _apic_global_enable();
    lapic_write_reg(LAPIC_TASKPR_REG, 0);
    lapic_write_reg(LAPIC_DEST_FMT_REG, 0xFFFFFFFF);
    debugf_debug("LAPIC is globally enabled and MSR is now %#llx\n",
                 _cpu_get_msr(0x1b));
    lapic_write_reg(LAPIC_SPURIOUS_REG, 0x1ff);
    debugf_debug("LAPIC_SPURIOUS_REG: %#lx\n",
                 lapic_read_reg(LAPIC_SPURIOUS_REG));

    lapic_write_reg(LAPIC_LINT0_REG, 0xfe);
    lapic_write_reg(LAPIC_LINT1_REG, 0xfe);
    lapic_write_reg(LAPIC_CMCI_REG, 0xfe);
    lapic_write_reg(LAPIC_ERR_REG, 0xfe);
    lapic_write_reg(LAPIC_PERFMON_COUNT_REG, 0xfe);
    lapic_write_reg(LAPIC_THERM_REG, 0xfe);
    lapic_write_reg(LAPIC_TIMER_REG, 0xfe);

    debugf_debug("LAPIC ID: %#hhx\n", lapic_get_id());
}

uint32_t lapic_timer_calibrate_pit(void) {
    // Set the divide value register
    lapic_write_reg(LAPIC_TIMER_DIV_REG, 0x3); // Divide by 16

    // Set initial counter to max value
    lapic_write_reg(LAPIC_TIMER_INIT_CNT, 0xFFFFFFFF);

    // Use PIT for calibration
    uint64_t start_tick = get_current_ticks();

    // Wait for a measurable time (10ms should be sufficient)
    pit_sleep(10);

    // Stop the timer by setting a mask bit
    lapic_write_reg(LAPIC_TIMER_REG, LAPIC_DISABLE);

    // Get elapsed time
    uint64_t end_tick      = get_current_ticks();
    uint64_t elapsed_ticks = end_tick - start_tick;

    // Read how many LAPIC timer ticks occurred
    uint32_t lapic_ticks = 0xFFFFFFFF - lapic_read_reg(LAPIC_TIMER_CURR_CNT);

    // Calculate ticks per ms
    uint32_t ticks_per_ms = lapic_ticks / elapsed_ticks;

    // Store this value for future use (can be static/global)
    return ticks_per_ms;
}

uint32_t calibrate_apic_timer_tsc(void) {

    lapic_write_reg(LAPIC_TIMER_INIT_CNT, 0xFFFFFFFF);

    tsc_sleep(100000);

    uint32_t end_count = lapic_read_reg(LAPIC_TIMER_CURR_CNT); 

    uint32_t elapsed_apic_ticks = 0xFFFFFFFF - end_count;

    uint64_t apic_timer_frequency = (uint64_t) (elapsed_apic_ticks * 10);

    uint32_t ticks_per_ms = apic_timer_frequency / 1000;

    debugf_debug("APIC Timer Frequency: %d ticks/ms\n", ticks_per_ms);

    return ticks_per_ms;
}

void lapic_timer_init(void) {
    // Calibrate the timer
    if (tsc) {
        lapic_timer_ticks_per_ms = calibrate_apic_timer_tsc();
    }
    lapic_timer_ticks_per_ms = lapic_timer_calibrate_pit();

    // Register the timer interrupt handler
    isr_registerHandler(LAPIC_TIMER_VECTOR + LAPIC_IRQ_OFFSET,
                        lapic_timer_handler);

    // Configure the timer in periodic mode
    // Set a reasonable value for your system (e.g., 10ms intervals)
    uint32_t count = lapic_timer_ticks_per_ms * 10; // 10ms intervals

    // Set the initial count
    lapic_write_reg(LAPIC_TIMER_INIT_CNT, count);

    // Set up the timer register with:
    // - The timer vector
    // - Periodic mode (bit 17)
    // - Not masked (bit 16 = 0)
    lapic_write_reg(LAPIC_TIMER_REG,
                    LAPIC_TIMER_VECTOR + LAPIC_IRQ_OFFSET | (1 << 17));

    debugf_debug("LAPIC Timer initialized: %u ticks per ms\n",
                 lapic_timer_ticks_per_ms);
}
// Add to apic.c
void lapic_timer_handler(registers_t *regs) {

    if (get_current_ticks() >= MAX_APIC_TICKS) set_ticks(0);

    UNUSED(regs);
    // Handle the timer interrupt
    // This could trigger your scheduler or update a time counter

    // Update system ticks (similar to your PIT handler)
    set_ticks(get_current_ticks() + 1);

    // If you have per-CPU scheduling, you would handle it here
    // process_handler(regs);

    // Send EOI
    lapic_send_eoi();
}
