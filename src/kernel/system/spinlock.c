#include "spinlock.h"

void spinlock_acquire(atomic_flag *lock) {
    while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE))
        ;
}

void spinlock_release(atomic_flag *lock) {
    __atomic_clear(lock, __ATOMIC_RELEASE);
}
