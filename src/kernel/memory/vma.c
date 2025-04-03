#include "vma.h"

#include <stdio.h>

#include "paging/paging.h"
#include "pmm.h"

#include "util/string.h"
#include "util/util.h"

#include <spinlock.h>

// @param map_allocation Tells the allocator if the found region needs to be
// mapped
// @param phys optional parameter, maps the newly allocated virtual address to
// such physical address
void *vma_alloc(vmm_context_t *ctx, size_t pages, bool map_allocation,
                void *phys) {

    void *ptr = NULL;

    virtmem_object_t *cur_vmo = ctx->root_vmo;
    virtmem_object_t *new_vmo;

    for (; cur_vmo != NULL; cur_vmo = cur_vmo->next) {
#ifdef VMM_DEBUG
        debugf_debug("Checking for available memory\n");
        vmo_dump(cur_vmo);
#endif

        if ((cur_vmo->len >= pages) && (BIT_GET(cur_vmo->flags, 8) == 0)) {

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

    cur_vmo = split_vmo_at(cur_vmo, pages);
    FLAG_SET(cur_vmo->flags, VMO_ALLOCATED);
    ptr = (void *)(cur_vmo->base);

    if (map_allocation) {
        void *phys_to_map = phys != NULL ? phys : pmm_alloc_pages(pages);
        map_region_to_page(ctx->pml4_table, (uint64_t)phys_to_map,
                           (uint64_t)ptr, (uint64_t)(pages * PFRAME_SIZE),
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

    if (unmap_allocation) {
        // find the physical address of the VMO
        uint64_t phys = pg_virtual_to_phys(ctx->pml4_table, cur_vmo->base);
        pmm_free((void *)phys, cur_vmo->len);
        unmap_region(ctx->pml4_table, cur_vmo->base,
                     (cur_vmo->len * PFRAME_SIZE));
    } else {
#ifdef VMM_DEBUG
        debugf_debug("We don't have to unmap\n");
#endif
    }
}
