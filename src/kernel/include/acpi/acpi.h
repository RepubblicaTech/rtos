#ifndef ACPI_H
#define ACPI_H 1

#include <stdint.h>
#include <stddef.h>

typedef struct acpi_sdt_header {
	char signature[4];

	uint32_t length;
	uint8_t revision;
	uint8_t checksum;

	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;

	uint32_t creator_id;
	uint32_t creator_revision;
} acpi_sdt_header;

typedef struct __attribute__((packed)) RSDT {
	acpi_sdt_header header;
	uint32_t sdt_pointer[];
} RSDT;

typedef struct __attribute__((packed)) XSDT {
	acpi_sdt_header header;
	uint64_t sdt_pointer[];
} XSDT;

uint64_t* get_root_sdt();
acpi_sdt_header* find_table(acpi_sdt_header* root_header, const char* signature);

void acpi_init();

#endif
