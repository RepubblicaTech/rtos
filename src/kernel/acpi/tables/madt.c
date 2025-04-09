#include "madt.h"

#include <stddef.h>
#include <stdio.h>

#include <util/string.h>

#include <uacpi/acpi.h>
#include <uacpi/tables.h>
#include <uacpi/uacpi.h>

#include <memory/heap/heap.h>

#include <kernel.h>

#include <cpu.h>

int madt_init() {
    struct acpi_madt *madt = kmalloc(sizeof(struct acpi_madt));

    /* uacpi_status ret = uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE,
    madt); if (uacpi_likely_error(ret)) { debugf_warn("Couldn't find MADT
    table!"); return -1;
    } */

    return 0;
}
