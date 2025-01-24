#ifndef TIME_H
#define TIME_H 1

#include <stdint.h>

#include <isr.h>

uint64_t get_current_ticks();
void set_ticks(uint64_t new_ticks);

#endif