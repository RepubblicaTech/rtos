#include "vmm.h"
#include "acpi/acpi.h"
#include "paging/paging.h"
#include "pmm.h"

#include <stdint.h>

#include <util/string.h>
#include <util/util.h>

#include <kernel.h>

#include <stdio.h>

#include <cpu.h>

vmm_context_t* current_vmm_ctx;

void vmm_switch_ctx(vmm_context_t* new_ctx) {
	current_vmm_ctx = new_ctx;
}

// imagine making a function to print stuff that you're going to barely use LMAO
void vmo_dump(virtmem_object_t *vmo) {
    debugf_debug("VMO %p\n", vmo);
    debugf_debug("\tprev: %p\n", vmo->prev);
    debugf_debug("\tbase: %#llx\n", vmo->base);
    debugf_debug("\tlen %zu\n", vmo->len);
    debugf_debug("\tflags: %#llb\n", vmo->flags);
    debugf_debug("\tnext: %p\n", vmo->next);
}

<<<<<<< HEAD
virtmem_object_t* vmo_init(uint64_t base, size_t length, uint64_t flags) {
	virtmem_object_t* vmo = (virtmem_object_t*)PHYS_TO_VIRTUAL(pmm_alloc(sizeof(virtmem_object_t)));
=======
virtmem_object_t *vmo_init(uint64_t base, size_t length, uint64_t flags) {
    virtmem_object_t *vmo =
        (virtmem_object_t *)PHYS_TO_VIRTUAL(fl_alloc(sizeof(virtmem_object_t)));
>>>>>>> 1ed5a09bf09a1b3305b7b4c24af03169c276b820

    vmo->base  = base;
    vmo->len   = length;
    vmo->flags = flags;

    vmo->next = NULL;
    vmo->prev = NULL;

    return vmo;
}

<<<<<<< HEAD
void vmo_destroy(virtmem_object_t* vmo) {
<<<<<<< Updated upstream
	pmm_free((void*)vmo);
=======
void vmo_destroy(virtmem_object_t *vmo) {
    fl_free((void *)vmo);
>>>>>>> 1ed5a09bf09a1b3305b7b4c24af03169c276b820

    vmo = NULL;
<<<<<<< Updated upstream
=======
=======
	pmm_free(vmo);
>>>>>>> Stashed changes
>>>>>>> Stashed changes
}

<<<<<<< HEAD
vmm_context_t* vmm_ctx_init(uint64_t* pml4, uint64_t flags) {
	vmm_context_t* ctx = (vmm_context_t*)PHYS_TO_VIRTUAL(pmm_alloc(sizeof(vmm_context_t)));

	if (pml4 == NULL) pml4 = (uint64_t*)pmm_alloc(PMLT_SIZE);
=======
vmm_context_t *vmm_ctx_init(uint64_t *pml4, uint64_t flags) {
    vmm_context_t *ctx =
        (vmm_context_t *)PHYS_TO_VIRTUAL(fl_alloc(sizeof(vmm_context_t)));
>>>>>>> 1ed5a09bf09a1b3305b7b4c24af03169c276b820

    ctx->pml4_table = pml4;
    ctx->root_vmo   = vmo_init(0, PMLT_SIZE, flags);

    return ctx;
<<<<<<< Updated upstream
=======
}

void vmm_ctx_destroy(vmm_context_t* ctx) {
	if (VIRT_TO_PHYSICAL(ctx->pml4_table) == (uint64_t)cpu_get_cr(3)) {
	} else {
		kprintf_warn("Attempted to destroy a pagemap that's currently in use. Skipping\n");
		return;
	}

	for (virtmem_object_t* i = ctx->root_vmo; i != NULL; i = i->next)
	{
		vmo_destroy(i);
	}
	
	// TODO: unmap all the page frames
	pmm_free(ctx->pml4_table);
	pmm_free(ctx);

	ctx->pml4_table = NULL;
	ctx = NULL;
>>>>>>> Stashed changes
}

uint64_t vmo_to_page_flags(uint64_t vmo_flags) {
    uint64_t pg_flags = 0x0;

    if (vmo_flags & VMO_PRESENT)
        pg_flags |= PMLE_PRESENT;
    if (vmo_flags & VMO_RW)
        pg_flags |= PMLE_WRITE;
    if (vmo_flags & VMO_USER)
        pg_flags |= PMLE_USER;

    return pg_flags;
}

uint64_t page_to_vmo_flags(uint64_t pg_flags) {
    uint64_t vmo_flags = 0x0;

    if (pg_flags & PMLE_PRESENT)
        vmo_flags |= VMO_PRESENT;
    if (pg_flags & PMLE_WRITE)
        vmo_flags |= VMO_RW;
    if (pg_flags & PMLE_USER)
        vmo_flags |= VMO_USER;

    return vmo_flags;
}

virtmem_object_t *split_vmo_at(virtmem_object_t *src_vmo, size_t len) {
    virtmem_object_t *new_vmo;

    if (src_vmo->len - len <= 0) {
        return src_vmo; // we are not going to split it
    }

<<<<<<< Updated upstream
=======
<<<<<<< Updated upstream
>>>>>>> Stashed changes
    new_vmo = vmo_init(src_vmo->base + (uint64_t)(len * PMLT_SIZE),
                       src_vmo->len - len, src_vmo->flags);
    /*
    src_vmo		  new_vmo
    [     [                        ]
    0	  0+len					   X
    */
<<<<<<< Updated upstream
=======
=======
	new_vmo = vmo_init(src_vmo->base + (uint64_t)(len * PMLT_SIZE), src_vmo->len - len, src_vmo->flags);
	/* for some reason

	src_vmo		  new_vmo
	[     [                        ]
	0	  0+len					   X
	*/
>>>>>>> Stashed changes
>>>>>>> Stashed changes

#ifdef VMM_DEBUG
    debugf_debug("VMO %p has been split at (virt)%#llx\n", src_vmo,
                 src_vmo->base + (uint64_t)(len * PMLT_SIZE));
#endif

    src_vmo->len = len;

    if (src_vmo->next != NULL)
        new_vmo->next = src_vmo->next;
    src_vmo->next = new_vmo;
    new_vmo->prev = src_vmo;
    /*
    src_vmo		  new_vmo
    [     ]<-->[                        ]-->...
    0	       0+len					X
    */

    return src_vmo;
}

// Initializes a simple 4KiB VMO and loads the contexts' PML4 into CR3
// Assumes the CTX has been initialized with vmm_ctx_init()
// --- DON'T USE THIS WITH THE KERNEL CTX!!! ---
void vmm_init(vmm_context_t *ctx) {
    for (virtmem_object_t *i = ctx->root_vmo; i != NULL; i = i->next) {
        // every VMO will have the same flags as the root one
        i->flags = ctx->root_vmo->flags;

<<<<<<< HEAD
		// mapping will be done on VMA_ALLOC
		// void *ptr = (void*)PHYS_TO_VIRTUAL(pmm_alloc(PMLT_SIZE));
		// map_region_to_page(ctx->pml4_table, (uint64_t)ptr, i->base, i->len, vmo_to_page_flags(ctx->root_vmo->flags));
	}
=======
        // mapping will be done on VMA_ALLOC
        // void *ptr = (void*)PHYS_TO_VIRTUAL(fl_alloc(PMLT_SIZE));
        // map_region_to_page(ctx->pml4_table, (uint64_t)ptr, i->base, i->len,
        // vmo_to_page_flags(ctx->root_vmo->flags));
    }
>>>>>>> 1ed5a09bf09a1b3305b7b4c24af03169c276b820

    // map the higher half
    // (0xffff80000... - virtual kernel end (0xffffffff))
    uint64_t *k_pml4 = (uint64_t *)PHYS_TO_VIRTUAL(get_kernel_pml4());
    for (int i = 256; i < 512; i++) {
        ctx->pml4_table[i] = k_pml4[i];
    }

<<<<<<< Updated upstream
    _load_pml4((uint64_t *)VIRT_TO_PHYSICAL(ctx->pml4_table));
=======
<<<<<<< Updated upstream
    _load_pml4((uint64_t *)VIRT_TO_PHYSICAL(ctx->pml4_table));
=======
	// _load_pml4((uint64_t*)VIRT_TO_PHYSICAL(ctx->pml4_table));
>>>>>>> Stashed changes
>>>>>>> Stashed changes
}
