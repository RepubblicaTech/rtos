#ifndef GDT_H
#define GDT_H 1

#include <stdint.h>

extern uint16_t GDT_CODE_SEGMENT;
extern uint16_t GDT_DATA_SEGMENT;

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t limit_high_and_flags;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t size;
    gdt_entry_t* pointer;
} __attribute__((packed)) gdt_pointer_t;

void gdt_init();

#endif
