#include "pit.h"

#include <io.h>
#include <stdio.h>

#include <util/util.h>

#include <time.h>

<<<<<<< Updated upstream
=======
<<<<<<< Updated upstream
>>>>>>> Stashed changes
void pit_tick(registers_t *regs) {
    // debugf(".");
    set_ticks(get_current_ticks() + 1);
}

=======
>>>>>>> Stashed changes
void pit_sleep(uint32_t millis) {
    uint64_t last_tick = get_current_ticks() + millis;
<<<<<<< Updated upstream
    while (get_current_ticks() < last_tick)
        ;
=======
    while (get_current_ticks() < last_tick);
>>>>>>> Stashed changes
}

void pit_init(const uint32_t freq) {
    set_ticks(0);

<<<<<<< Updated upstream
    irq_registerHandler(0, pit_tick);
=======
<<<<<<< Updated upstream
    irq_registerHandler(0, pit_tick);
=======
	irq_registerHandler(0, timer_tick);
>>>>>>> Stashed changes
>>>>>>> Stashed changes

    uint32_t divisor = CONST_PIT_CLOCK / freq;

    // --- 0x36 0011 0110 ---

    // 00: 	select channel 0
    // 11: 	access high AND low byte

    // 011: Mode 3 (square wave generator)
    // 0:	16-bit binary
    _outb(PIT_MODE_CMD, 0x36);
    // set the PIT frequency (freq/PIT's clock frequency)
    _outb(PIT_CH0_DATA, (uint8_t)(divisor & 0xFF));
    _io_wait();
    _outb(PIT_CH0_DATA, (uint8_t)((divisor >> 8) & 0xFF));
}
