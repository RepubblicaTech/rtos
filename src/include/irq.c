#include <irq.h>
#include <util/pic.h>

#include <isr.h>

#include <io/io.h>
#include <stdio.h>

#include <stddef.h>


#define PIC_REMAP_OFFSET        0x20

irq_handler irq_handlers[16];

extern void enable_interrupts();

void irqHandler(registers* regs) {
    int irq = regs->interrupt - PIC_REMAP_OFFSET;

    uint8_t pic_isr = pic_get_isr();
    uint8_t pic_irr = pic_get_irr();

    if (irq_handlers[irq] != NULL) {
        irq_handlers[irq](regs);                // tries to handle the interrupt
    } else {
        debugf("Unhandled IRQ %d  ISR=%X  IRR=%X...\n", irq, pic_isr, pic_irr);
    }

    pic_sendEOI(irq);
}

void irq_init() {
    pic_config(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    for (int i = 0; i < 16; i++) {
        isr_registerHandler(PIC_REMAP_OFFSET + i, irqHandler);
    }

    enable_interrupts();
    
}

void irq_registerHandler(int irq, irq_handler handler) {
    irq_handlers[irq] = handler;
}
