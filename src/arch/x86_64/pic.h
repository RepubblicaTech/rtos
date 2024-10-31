#ifndef PIC_H
#define PIC_H 1

#define PIC1_COMMAND	0x20
#define PIC1_DATA	    0x21
#define PIC2_COMMAND	0xA0
#define PIC2_DATA	    0xA1

#define PIC_EOI		    0x20    // End-of-interrupt command code

#define PIC_READ_IRR    0x0a    // OCW3 irq ready next CMD read
#define PIC_READ_ISR    0x0b    // OCW3 irq service next CMD read

#include <stdint.h>

void pic_sendEOI(uint8_t irq);
void pic_config(int offset1, int offset2);
void irq_mask(uint8_t irq_line);
void irq_unmask(uint8_t irq_line);
void pic_disable(void);
uint16_t pic_get_irr(void);
uint16_t pic_get_isr(void);

#endif