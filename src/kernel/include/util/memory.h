#ifndef MEMORY_H
#define MEMORY_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <limine.h>
#include <kernel.h>

extern struct bootloader_data limine_parsed_data;

#define PHYS_TO_VIRTUAL(ADDR)       (limine_parsed_data.hhdm_offset | (uint64_t)ADDR)
#define VIRT_TO_PHYSICAL(ADDR)      (ADDR - limine_parsed_data.hhdm_offset)

/*
    GCC and Clang reserve the right to generate calls to the following
    4 functions even if they are not directly called.
    Implement them as the C specification mandates.
    DO NOT remove or rename these functions, or stuff will eventually break!
    They CAN be moved to a different .c file.
*/
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

static const char* const memory_block_type[] = {
    "USABLE",
    "RESERVED",
    "ACPI_RECLAIMABLE",
    "ACPI_NVS",
    "BAD",
    "BOOTLOADER_RECLAIMABLE",
    "KERNEL_AND_MODULES",
    "FRAMEBUFFER"
};

#endif
