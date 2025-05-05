#ifndef MADT_H
#define MADT_H 1

#include <acpi/acpi.h>
#include <uacpi/uacpi.h>

#define MADT_ENTRY_LAPIC               0
#define MADT_ENTRY_IOAPIC              1
#define MADT_ENTRY_IOAPIC_INT_OVERRIDE 2
#define MADT_ENTRY_NMI_SOURCE          3
#define MADT_ENTRY_LAPIC_ADDR_OVVERRIDE                                        \
    5 // it's the 64-bit address of the LAPIC
#define MADT_ENTRY_LAPIC_NMI    4
#define MADT_ENTRY_LOCAL_X2APIC 9

#define MADT_RECORD_OFFSET 0x2C

#endif