#include <spinlock.h>
#include <stddef.h>

#include <memory/vma.h>

// liballoc will now work on the current VMM context loaded here
extern vmm_context_t* current_vmm_ctx;

atomic_flag ATOMIC_LIBALLOC = ATOMIC_FLAG_INIT;

int liballoc_lock() {
	spinlock_acquire(&ATOMIC_LIBALLOC);
	return 0;
}

int liballoc_unlock() {
	spinlock_release(&ATOMIC_LIBALLOC);
	return 0;
}

void* liballoc_alloc(size_t pages) {
	return vma_alloc(current_vmm_ctx, (pages), false);
}

int liballoc_free(void* ptr, size_t pages) {
	pages = 0;
	vma_free(current_vmm_ctx, ptr, false);
}
