#ifndef MADT_H
#define MADT_H 1

#include <acpi/acpi.h>

typedef struct madt_record {
    uint8_t entry_type;
    uint8_t entry_length;
} madt_record;

#define MADT_ENTRY_LAPIC               0
#define MADT_ENTRY_IOAPIC              1
#define MADT_ENTRY_IOAPIC_INT_OVERRIDE 2
#define MADT_ENTRY_NMI_SOURCE          3
#define MADT_ENTRY_LAPIC_ADDR_OVVERRIDE                                        \
    5 // it's the 64-bit address of the LAPIC
#define MADT_ENTRY_LAPIC_NMI    4
#define MADT_ENTRY_LOCAL_X2APIC 9

typedef struct madt_lapic {
    madt_record record;
    uint8_t acpi_processor_id;
    uint8_t id;
    uint32_t flags;
} madt_lapic;

typedef struct madt_ioapic {
    madt_record record;
    uint8_t id;
    uint8_t reserved;
    uint32_t address;
    uint32_t global_sys_interrupt_base;
} madt_ioapic;

typedef struct madt_ioapic_int_override {
    madt_record record;
    uint8_t bus_source;
    uint8_t legacy_irq_source;
    uint32_t ioapic_irq;
    uint16_t flags;

} madt_ioapic_int_override;

typedef struct madt_lapic_addr_ovveride {
    madt_record record;
    uint16_t reserved;
    uint64_t physical_base; // physical 64-bit address of the LAPIC
} madt_lapic_addr_ovveride;

typedef struct madt {
    acpi_sdt_header header;

    uint32_t lapic_control_address;
    uint32_t lapic_flags;

    uint8_t *entries;
} madt;

madt *get_madt();

void madt_devices_init();
madt_record *find_record(madt *madt_h, const uint8_t entry_type);

#endif