#include "time.h"

#include <scheduler/scheduler.h>
#include <util/util.h>

#include <stdio.h>

uint64_t ticks;

uint64_t get_current_ticks() {
    return ticks;
}

void set_ticks(uint64_t new_ticks) {
    ticks = new_ticks;
}

void timer_tick(registers_t *regs) {
    UNUSED(regs);
    // debugf(".");
    set_ticks(get_current_ticks() + 1);
}

void sched_timer_tick(registers_t *regs) {
    UNUSED(regs);
    // debugf(".");
    set_ticks(get_current_ticks() + 1);
}