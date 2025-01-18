#ifndef SPINLOCK_H
#define SPINLOCK_H 1

#include <stdint.h>
#include <stdatomic.h>

void spinlock_acquire(atomic_flag* lock);
void spinlock_release(atomic_flag* lock);

#endif