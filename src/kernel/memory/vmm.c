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

vmm_context_t *current_vmm_ctx;

vmm_context_t *get_current_ctx() {
    return current_vmm_ctx;
}

void vmm_switch_ctx(vmm_context_t *new_ctx) {
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

virtmem_object_t *vmo_init(uint64_t base, size_t length, uint64_t flags) {
    virtmem_object_t *vmo = (virtmem_object_t *)PHYS_TO_VIRTUAL(pmm_alloc_pages(
        ROUND_UP(sizeof(virtmem_object_t), PFRAME_SIZE) / PFRAME_SIZE));

    vmo->base  = base;
    vmo->len   = length;
    vmo->flags = flags;

    vmo->next = NULL;
    vmo->prev = NULL;

    return vmo;
}

vmm_context_t *vmm_ctx_init(uint64_t *pml4, uint64_t flags) {
    vmm_context_t *ctx = (vmm_context_t *)PHYS_TO_VIRTUAL(pmm_alloc_pages(
        ROUND_UP(sizeof(vmm_context_t), PFRAME_SIZE) / PFRAME_SIZE));

    if (pml4 == NULL) {
        pml4 = (uint64_t *)PHYS_TO_VIRTUAL(pmm_alloc_page());
    }

    ctx->pml4_table = pml4;
    ctx->root_vmo   = vmo_init(0, 1, flags);

    return ctx;
}

void vmm_ctx_destroy(vmm_context_t *ctx) {
    if (VIRT_TO_PHYSICAL(ctx->pml4_table) == (uint64_t)cpu_get_cr(3)) {
    } else {
        kprintf_warn("Attempted to destroy a pagemap that's currently in use. "
                     "Skipping\n");
        return;
    }

    for (virtmem_object_t *i = ctx->root_vmo; i != NULL; i = i->next) {
        uint64_t phys = pg_virtual_to_phys(ctx->pml4_table, i->base);
        pmm_free((void *)PHYS_TO_VIRTUAL(phys), i->len);

        pmm_free(i,
                 ROUND_UP(sizeof(virtmem_object_t), PFRAME_SIZE) / PFRAME_SIZE);
    }

    // TODO: unmap all the page frames
    pmm_free(ctx->pml4_table, 1);
    pmm_free(ctx, ROUND_UP(sizeof(vmm_context_t), PFRAME_SIZE) / PFRAME_SIZE);

    ctx->pml4_table = NULL;
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

    new_vmo = vmo_init(src_vmo->base + (uint64_t)(len * PFRAME_SIZE),
                       src_vmo->len - len, src_vmo->flags);
    /*
    src_vmo		  new_vmo
    [     [                        ]
    0	  0+len					   X
    */

#ifdef VMM_DEBUG
    debugf_debug("VMO %p has been split at (virt)%#llx\n", src_vmo,
                 src_vmo->base + (uint64_t)(len * PFRAME_SIZE));
#endif

    src_vmo->len = len;

    if (src_vmo->next != NULL) {
        new_vmo->next = src_vmo->next;
        src_vmo->next = new_vmo;
        new_vmo->prev = src_vmo;
    }

    /*
    src_vmo		  new_vmo
    [     ]<-->[                        ]-->...
    0	       0+len					X
    */

    return src_vmo;
}

void pagemap_copy_to(uint64_t *non_kernel_pml4) {

    uint64_t *k_pml4 = (uint64_t *)PHYS_TO_VIRTUAL(get_kernel_pml4());

    if ((uint64_t *)PHYS_TO_VIRTUAL(non_kernel_pml4) == k_pml4)
        return;

    for (int i = 0; i < 512; i++) {
        // debugf("Copying %p[%d](%#llx) to %p[%d]\n", k_pml4, i, k_pml4[i],
        //        non_kernel_pml4, i);

        ((uint64_t *)PHYS_TO_VIRTUAL(non_kernel_pml4))[i] = k_pml4[i];
    }
}

// Assumes the CTX has been initialized with vmm_ctx_init()
void vmm_init(vmm_context_t *ctx) {
    for (virtmem_object_t *i = ctx->root_vmo; i != NULL; i = i->next) {
        // every VMO will have the same flags as the root one
        i->flags = ctx->root_vmo->flags;

        // mapping will be done on vma_alloc
    }

    pagemap_copy_to(ctx->pml4_table);
}
