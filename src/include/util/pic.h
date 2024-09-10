#ifndef PIC_H
#define PIC_H 1

#include <stdint.h>

void pic_sendEOI(uint8_t irq);
void pic_config(int offset1, int offset2);
void irq_mask(uint8_t irq_line);
void irq_unmask(uint8_t irq_line);
void pic_disable(void);
uint16_t pic_get_irr(void);
uint16_t pic_get_isr(void);

#endif