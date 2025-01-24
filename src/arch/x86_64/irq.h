#ifndef IRQ_H
#define IRQ_H 1

#include <stdint.h>

#include "isr.h"

typedef void (*irq_handler)(registers_t *regs);

extern void _enable_interrupts();
extern void _disable_interrupts();

void irq_init();
void irq_registerHandler(int irq, irq_handler handler);

#endif
