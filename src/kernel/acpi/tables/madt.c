#include "madt.h"

#include <stddef.h>
#include <stdio.h>

#include <util/string.h>

#include <uacpi/acpi.h>
#include <uacpi/tables.h>
#include <uacpi/uacpi.h>

#include <memory/heap/beap.h>

#include <kernel.h>

#include <cpu.h>

int madt_init() {
    struct acpi_madt *madt = kmalloc(sizeof(struct acpi_madt));

    /* --- TODOs ---

    1. Get the table
    2. Get required info for L/IO-APIC (ISOs, base address(es), etc.)
    3. MAKE SURE TO free()

    */

    return 0;
}
