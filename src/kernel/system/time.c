#include "time.h"

uint64_t ticks;

uint64_t get_current_ticks() {
	return ticks;
}

void set_ticks(uint64_t new_ticks) {
	ticks = new_ticks;
}
