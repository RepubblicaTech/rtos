#ifndef ARCH_H
#define ARCH_H 1

#include <stdint.h>

void sleep(unsigned long ms);

/* initialises basic arch-specific components:

    - GDT
    - IDT
    - ISRs
    - PIC and IRQs
    - PIT (or TSC if supported)

*/
void arch_base_init();

// sets the given `marker` to the current time, either TSC/PIT/any timer that
// the system uses
void mark_time(uint64_t *marker);

// returns the time passed between the given start marker and the current time,
// based on the clock used currently in your system
uint64_t get_ms(uint64_t start);

#endif