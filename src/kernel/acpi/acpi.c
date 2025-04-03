#include "acpi/acpi.h"

#include "uacpi/acpi.h"
#include "uacpi/uacpi.h"

#include <stdio.h>

int uacpi_init() {
    uacpi_status ret = uacpi_initialize(UACPI_FLAG_NO_OSI);
    if (uacpi_likely_error(ret)) {
        uacpi_kernel_log(UACPI_LOG_ERROR,
                         "Couldn't initialize basic ACPI components!\n");

        return -1;
    }

    return 0;
}
