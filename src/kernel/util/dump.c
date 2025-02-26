#include "dump.h"
#include <stdio.h>

void hex_dump(const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *)data;
    size_t remainder     = size % 16;
    size_t padded_size   = remainder ? size + (16 - remainder) : size;

    for (size_t i = 0; i < padded_size; i += 16) {
        // Print hex bytes
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                kprintf("%02X ", bytes[i + j]);
            } else {
                kprintf("   "); // Padding for alignment
            }
        }

        kprintf("|");

        // Print ASCII representation
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                char c = bytes[i + j];
                kprintf("%c", is_printable(c) ? c : '.');
            } else {
                kprintf("."); // Fill with dots for missing characters
            }
        }

        kprintf("\n");
    }
}