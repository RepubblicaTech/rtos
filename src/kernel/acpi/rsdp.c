#include <acpi/rsdp.h>

#include <kernel.h>

#include <memory/pmm.h>

#include <util/string.h>

static struct bootloader_data *limine_data;
extern void _hcf();

int is_xsdp(sdt_pointer *pointer) {
    return (pointer->revision > 0);
}

// returns the root system table pointer
sdt_pointer *get_rsdp() {
    // we need the address that points to the RSDP
    limine_data = get_bootloader_data();
    uint64_t *v_rsdp_addr =
        (uint64_t *)PHYS_TO_VIRTUAL(limine_data->rsdp_table_address);

    return (sdt_pointer *)v_rsdp_addr;
}
