/*
	Root System Description Pointer table structure

	(C) RepubblicaTech 2024
*/

#ifndef RSDP_H
#define RSDP_H 1

#include <stdint.h>

typedef struct sdt_pointer {
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;

	uint32_t p_rsdt_address;      // 32-bit physical address of the RSDT table

	// XSDT stuff (for ACPI >=2.0)
	uint32_t length;
 	uint64_t p_xsdt_address;
 	uint8_t extended_checksum;
 	uint8_t reserved[3];
} sdt_pointer;

// given an SDP, simply says if it's an XSDP or not
int is_xsdp(sdt_pointer* pointer);

sdt_pointer* get_rsdp();

#endif
