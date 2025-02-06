#include "vma.h"

#include <stdio.h>

#include "paging/paging.h"
#include "pmm.h"

#include "util/string.h"
#include "util/util.h"

// @param map_allocation Tells the allocator if the found region needs to be
// mapped (disable for kernel VMA, enable for other VMAs)
void *vma_alloc(vmm_context_t *ctx, size_t pages, bool map_allocation) {
    void *ptr = NULL;

    virtmem_object_t *cur_vmo = ctx->root_vmo;
    virtmem_object_t *new_vmo;

    for (; cur_vmo != NULL; cur_vmo = cur_vmo->next) {
#ifdef VMM_DEBUG
        debugf_debug("Checking for available memory\n");
        vmo_dump(cur_vmo);
#endif

        if ((cur_vmo->len >= pages) && (cur_vmo->flags & ~(VMO_ALLOCATED))) {

#ifdef VMM_DEBUG
            debugf_debug("Well, we've got enough memory :D\n");
#endif
            break;
        }

#ifdef VMM_DEBUG
        debugf_debug("Current VMO is either too small or already "
                     "allocated. Skipping...\n");
#endif
        if (cur_vmo->next == NULL) {
            new_vmo =
                vmo_init(cur_vmo->base + (uint64_t)(cur_vmo->len * PFRAME_SIZE),
                         pages, cur_vmo->flags & ~(VMO_ALLOCATED));
            cur_vmo->next = new_vmo;
            new_vmo->prev = cur_vmo;
#ifdef VMM_DEBUG
            debugf_debug("VMO %p created successfully. Proceeding to next "
                         "iteration\n",
                         new_vmo);
#endif
        }
    }

    if (cur_vmo == NULL) {
        kprintf_panic("VMM ran out of memory and is not able to request it "
                      "from the PMM.\n");
        _hcf();
    }

    ptr = (void *)(cur_vmo->base);
    if (map_allocation) {
        void *phys = pmm_alloc_pages(pages);
        map_region_to_page(ctx->pml4_table, (uint64_t)phys, (uint64_t)ptr,
                           (uint64_t)(pages * PFRAME_SIZE),
                           vmo_to_page_flags(cur_vmo->flags));
    } else {
#ifdef VMM_DEBUG
        debugf_debug("No need to map the region\n");
#endif
    }

    cur_vmo         = split_vmo_at(cur_vmo, pages);
    cur_vmo->flags |= VMO_ALLOCATED;
    ptr             = (void *)(cur_vmo->base);
    if (map_allocation) {
        void *phys = pmm_alloc_pages(pages);
        map_region_to_page(ctx->pml4_table, (uint64_t)phys, (uint64_t)ptr,
                           (uint64_t)(pages * PFRAME_SIZE),
                           vmo_to_page_flags(cur_vmo->flags));
    } else {
#ifdef VMM_DEBUG
        debugf_debug("No need to map the region\n");
#endif
    }

#ifdef VMM_DEBUG
    debugf_debug("Returning pointer %p\n", ptr);
#endif
    return ptr;
}

void vma_free(vmm_context_t *ctx, void *ptr, bool unmap_allocation) {
#ifdef VMM_DEBUG
    debugf_debug("Deallocating pointer %p\n", ptr);
#endif

    virtmem_object_t *cur_vmo = ctx->root_vmo;
    for (; cur_vmo != NULL; cur_vmo = cur_vmo->next) {
#ifdef VMM_DEBUG
        vmo_dump(cur_vmo);
#endif
        if ((uint64_t)ptr == cur_vmo->base) {
            break;
        }
#ifdef VMM_DEBUG
        debugf_debug("Pointer and vmo->base don't match. Skipping\n");
#endif
    }

    if (cur_vmo == NULL) {
#ifdef VMM_DEBUG
        debugf_debug(
            "Tried to deallocate a non-existing pointer. Quitting...\n");
#endif
        return;
    }

    FLAG_UNSET(cur_vmo->flags, VMO_ALLOCATED);

    // we'll remove the VMO from the list

    // find the physical address of the VMO
    uint64_t phys = pg_virtual_to_phys(ctx->pml4_table, cur_vmo->base);
    pmm_free((void *)PHYS_TO_VIRTUAL(phys), cur_vmo->len);

    virtmem_object_t *prev_vmo, *next_vmo;
    prev_vmo = cur_vmo->prev;
    next_vmo = cur_vmo->next;
    // delete the VMO from the list

    if (prev_vmo != NULL)
        prev_vmo->next = next_vmo;
    if (next_vmo != NULL)
        next_vmo->prev = prev_vmo;

    if (unmap_allocation) {
        pmm_free((void *)PHYS_TO_VIRTUAL(
                     pg_virtual_to_phys(ctx->pml4_table, cur_vmo->base)),
                 cur_vmo->len);
        unmap_region(ctx->pml4_table, cur_vmo->base,
                     (cur_vmo->len * PFRAME_SIZE));
    } else {
#ifdef VMM_DEBUG
        debugf_debug("We don't have to unmap\n");
#endif
    }

    pmm_free(cur_vmo,
             ROUND_UP(sizeof(virtmem_object_t), PFRAME_SIZE) / PFRAME_SIZE);
}
