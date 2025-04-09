#include <mmio/apic/apic.h>
#include <mmio/apic/io_apic.h>

#include <kernel.h>

#include <stdio.h>

#include <util/string.h>

#include <acpi/tables/madt.h>
#include <mmio/mmio.h>

#include <time.h>

#include <io.h>

#include <memory/heap/heap.h>
#include <memory/pmm.h>

mmio_device *mmios;

static struct bootloader_data *limine_data;

uint64_t *ioapic_redtbl;
uint64_t *get_redtbl() {
    return ioapic_redtbl;
}

uint32_t ioapic_reg_read(uint8_t reg) {
    uint32_t volatile *regsel =
        (uint32_t volatile *)PHYS_TO_VIRTUAL(limine_data->p_ioapic_base);
    regsel[0] = reg;

    return regsel[4];
}

void ioapic_reg_write(uint8_t reg, uint32_t value) {
    uint32_t volatile *regsel =
        (uint32_t volatile *)PHYS_TO_VIRTUAL(limine_data->p_ioapic_base);
    regsel[0] = reg;

    regsel[4] = value;
}

irq_handler apic_irq_handlers[IOREDTBL_ENTRIES];

void apic_irq_handler(void *ctx) {
    registers_t *regs = ctx;

    int apic_irq = regs->interrupt - IOAPIC_IRQ_OFFSET;

    uint64_t apic_isr = lapic_read_reg(LAPIC_INSERVICE_REG);
    uint64_t apic_irr = lapic_read_reg(LAPIC_INT_REQ_REG);

    if (apic_irq_handlers[apic_irq] != NULL) {
        apic_irq_handlers[apic_irq](regs);
    } else {
        debugf_warn("Unhandled IRQ %d  ISR=%#llx  IRR=%#llx\n", apic_irq,
                    apic_isr, apic_irr);
    }

    lapic_send_eoi();
}

void apic_registerHandler(int irq, irq_handler handler) {
    apic_irq_handlers[irq] = handler;
}

void apic_unregisterHandler(int irq) {
    if (irq >= 0 && irq < IOREDTBL_ENTRIES) {
        apic_irq_handlers[irq] = NULL;
    }
}

// maps an I/O APIC IRQ to an interrupt that calls the handler if fired
// @param irq			- The hardware interrupt that'll be fired
// @param interrupt		- The interrupt vector that will be written to
// the I/O APIC
// @param handler		- The ISR that will be called when the interrupt
// gets fired

void ioapic_map_irq(int irq, int interrupt, irq_handler handler) {
    uint32_t redtble_lo = (0 << 16) |    // Unmask the entry
                          (0 << 11) |    // Destination mode
                          (0b000 << 8) | // Delivery mode
                          interrupt;     // interrupt vector
    ioapic_reg_write(0x10 + (2 * irq), redtble_lo);

    uint32_t redtble_hi = (lapic_get_id() << 24);
    ioapic_reg_write(0x10 + (2 * irq) + 1, redtble_hi);

    isr_registerHandler(interrupt, handler);
}

void ioapic_init() {
    /*
    limine_data = get_bootloader_data();

    mmio_device mmio_ioapic    = find_mmio(MMIO_APIC_SIG);
    limine_data->p_ioapic_base = mmio_ioapic.base;
    debugf_debug("I/O APIC Base address: %#lx\n", limine_data->p_ioapic_base);

    uint8_t ioapic_redir_entries = (ioapic_reg_read(0x01) >> 16) & 0xFF;
    debugf_debug("Redirection entries: %#hhu\n", ioapic_redir_entries);

    uint64_t *ioredtbl =
    (uint64_t *)kmalloc(sizeof(uint64_t) * ioapic_redir_entries);
    int redir = 0;
    for (int i = 0x10; i < 0x3f; i += 2) {
        ioredtbl[redir] = ioapic_reg_read(i) | (ioapic_reg_read(i + 1) << 31);
        redir++;
    }

    // now that we got the redirection entries, we should now get all IRQ
    // overrides
    madt_ioapic_int_override **ioapic_int_override =
    (madt_ioapic_int_override **)kmalloc(
        sizeof(madt_ioapic_int_override *) * ioapic_redir_entries);
        int iso_entry = 0;
        // 18/12/2024: Let' read and save all redirection entries :)

        for (int i = 0; i < iso_entry; i++) {
            debugf_debug("Interrupt override n.%d\n", i);
            debugf_debug("\tI/O APIC IRQ: %lu\n",
            ioapic_int_override[i]->ioapic_irq);
            debugf_debug("\tLegacy PIC IRQ: %hhu\n",
            ioapic_int_override[i]->legacy_irq_source);
            ioapic_map_irq(ioapic_int_override[i]->ioapic_irq,
            IOAPIC_IRQ_OFFSET +
            ioapic_int_override[i]->legacy_irq_source,
            apic_irq_handler);
        }

        for (int i = 1; i < ioapic_redir_entries; i++) {
            // to map the other normal interrupts, we'll just check if it's
    masked
            // or not
            if (ioapic_reg_read(0x10 + (2 * i)) & (1 << 16)) {
                ioapic_map_irq(i, i + IOAPIC_IRQ_OFFSET, apic_irq_handler);
            }
        }
        */
}
