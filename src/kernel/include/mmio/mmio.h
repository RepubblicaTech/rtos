#ifndef MMIO_H
#define MMIO_H 1

#include <stdint.h>
#include <stddef.h>

#define MMIO_MAX_DEVICES		3

typedef struct mmio_device {
	uint64_t base;
	size_t size;
	char* name;
} mmio_device;

mmio_device* get_mmio_devices();
mmio_device find_mmio(const char* sig);

void append_mmio(mmio_device device);

#endif