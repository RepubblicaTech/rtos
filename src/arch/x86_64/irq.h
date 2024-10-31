#ifndef IRQ_H
#define IRQ_H 1

#include <stdint.h>

#include "isr.h"

typedef void (*irq_handler)(registers* regs);

void irq_init();
void irq_registerHandler(int irq, irq_handler handler);


#endif
