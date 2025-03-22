#include "spinlock.h"
#include "smp/smp.h"
#include "stdio.h"

void spinlock_acquire(atomic_flag *lock) {
    unsigned int timeout = 1000000;

    while (atomic_flag_test_and_set(lock)) {
        if (--timeout == 0) {
            debugf_warn("Spinlock deadlock detected\n");
            spinlock_release(lock);
        }

        asm("pause");
    }
}

void spinlock_release(atomic_flag *lock) {
    atomic_flag_clear(lock);
}
