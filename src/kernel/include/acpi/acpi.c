#include "acpi.h"

#include "rsdp.h"
#include "tables/madt.h"
#include <util/string.h>

#include <stdio.h>

RSDT* rsdt;
XSDT* xsdt;

// returns the address of the R/XSDT
uint64_t* get_root_sdt() {
	sdt_pointer* sdp = get_rsdp();
	return (is_xsdp(sdp) ? (uint64_t*)sdp->p_xsdt_address : (uint64_t*)(uint64_t)sdp->p_rsdt_address);
}

int check_table_signature(acpi_sdt_header* table, const char* sig) {
	for (int i = 0; i < 4; i++)
	{
		if (table->signature[i] != sig[i])
			return table->signature[i] - sig[i];
	}

	return 0;
}

// given a R/XSDT header, the function returns a table that matches the given signature
acpi_sdt_header* find_table(acpi_sdt_header* root_header, const char* sig) {
	debugf_debug("Searching for table \"%s\"\n", sig);
	acpi_sdt_header* table;

	if (is_xsdp(get_rsdp())) {
		xsdt = (XSDT*)root_header;
		size_t entries = (xsdt->header.length - sizeof(xsdt->header)) / 8;

		for (size_t i = 0; i < entries; i++) {
			table = (acpi_sdt_header*)PHYS_TO_VIRTUAL(xsdt->sdt_pointer[i]);
			if (check_table_signature(table, sig) == 0)
				return table;
		}

	} else {
		rsdt = (RSDT*)root_header;
		int entries = (rsdt->header.length - sizeof(rsdt->header)) / 4;

		for (int i = 0; i < entries; i++) {
			table = (acpi_sdt_header*)PHYS_TO_VIRTUAL(rsdt->sdt_pointer[i]);
			if (check_table_signature(table, sig) == 0)
				return table;
		}
	}

	// if we are here, then we didn't find anything
	debugf_debug("Couldn't find table :c\n");
	return NULL;
}

void acpi_init() {
	sdt_pointer* rsdp = get_rsdp();

	debugf_debug("--- %s INFO ---\n", rsdp->revision > 0 ? "XSDP" : "RSDP");

	debugf_debug("Signature: %.8s\n", rsdp->signature);
	debugf_debug("Checksum: %hhu\n\n", rsdp->checksum);

	debugf_debug("OEM ID: %.6s\n", rsdp->oem_id);
	debugf_debug("Revision: %s\n", rsdp->revision > 0 ? "2.0 - 6.1" : "1.0");

	if (rsdp->revision > 0) {
		debugf_debug("Length: %lu\n", rsdp->length);
		debugf_debug("XSDT address: %#llp\n", rsdp->p_xsdt_address);
		debugf_debug("Extended checksum: %hhu\n", rsdp->extended_checksum);
	} else {
		debugf_debug("RSDT address: %#lp\n", rsdp->p_rsdt_address);
	}

	debugf_debug("--- %s END ---\n", rsdp->revision > 0 ? "XSDP" : "RSDP");

	// initialize the required tables
	madt_devices_init();
}
