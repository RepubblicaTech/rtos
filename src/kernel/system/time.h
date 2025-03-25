#ifndef TIME_H
#define TIME_H 1

#include <stdint.h>

#include <isr.h>

uint64_t get_current_ticks();
void set_ticks(uint64_t new_ticks);

void timer_tick(void *ctx);
void sched_timer_tick(void *ctx);

#endif