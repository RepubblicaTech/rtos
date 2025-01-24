#include "time.h"

uint64_t ticks;

uint64_t get_current_ticks() {
    return ticks;
}

void set_ticks(uint64_t new_ticks) {
    ticks = new_ticks;
<<<<<<< Updated upstream
=======
}

void timer_tick(registers_t* regs) {
	// debugf(".");
	set_ticks(get_current_ticks() + 1);

>>>>>>> Stashed changes
}
