#include "spinlock.h"

<<<<<<< Updated upstream
=======
<<<<<<< Updated upstream
>>>>>>> Stashed changes
void spinlock_acquire(atomic_flag *lock) {
    while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
        asm volatile("hlt");
    }
<<<<<<< Updated upstream
}

void spinlock_release(atomic_flag *lock) {
    atomic_flag_clear_explicit(lock, memory_order_release);
}
=======
}

void spinlock_release(atomic_flag *lock) {
    atomic_flag_clear_explicit(lock, memory_order_release);
}
=======
void spinlock_acquire(atomic_flag* lock) {
	while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire));
	
}

void spinlock_release(atomic_flag* lock) {
	atomic_flag_clear_explicit(lock, memory_order_release);
}
>>>>>>> Stashed changes
>>>>>>> Stashed changes
