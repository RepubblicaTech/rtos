/*
        Freelist memory allocator/manager

        (C) RepubblicaTech 2024
*/

#include "pmm.h"

#include <kernel.h>
#include <limine.h>

#include <util/string.h>
#include <util/util.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

int usable_entry_count;

extern struct limine_memmap_response *memmap_response;
extern void _hcf();

freelist_node *fl_head;

void pmm_init() {

    // array of nodes (used only on initialization)
    freelist_node *fl_nodes[limine_parsed_data.usable_entry_count];

    usable_entry_count = 0;
    for (uint64_t i = 0, temp = 0; i < memmap_response->entry_count; i++) {
        struct limine_memmap_entry *memmap_entry = memmap_response->entries[i];

        if (memmap_entry->type != LIMINE_MEMMAP_USABLE)
            continue;

        usable_entry_count++;
    }
    // create a freelist entry that points to the start of each usable address
    // and link them
    // @note temp is a temporary counter for the fl_nodes array
    for (uint64_t i = 0, temp = 0; i < memmap_response->entry_count; i++) {
        struct limine_memmap_entry *memmap_entry = memmap_response->entries[i];

        if (memmap_entry->type != LIMINE_MEMMAP_USABLE)
            continue;

        freelist_node *fl_node =
            (freelist_node *)PHYS_TO_VIRTUAL(memmap_entry->base);
        fl_node->length = memmap_entry->length;

        if (temp == 0)
            fl_head = fl_node;

        fl_nodes[temp] = fl_node;

        temp++;
    }

    for (int i = 0; i < usable_entry_count; i++) {
        if (i < usable_entry_count - 1) {
            fl_nodes[i]->next = fl_nodes[i + 1];
        } else {
            fl_nodes[i]->next = NULL;
        }
    }

    kprintf_info("Found %d usable regions\n", usable_entry_count);

    // prints all nodes
    for (freelist_node *fl_node = fl_head; fl_node != NULL;
         fl_node                = fl_node->next) {
        debugf_debug("ENTRY n. %p\n", fl_node);
        debugf_debug("\tlength: %#llx\n", fl_node->length);
        debugf_debug("\tnext: %p\n", fl_node->next);
    }
}

// Returns the count of the entries.
int get_freelist_entry_count() {
    return usable_entry_count;
}

/*
        "Refreshes" the list of entries

        @returns head of nodes
*/
freelist_node *fl_update_nodes() {
    usable_entry_count = 0;
    for (freelist_node *i = fl_head; i != NULL;
         i                = i->next, usable_entry_count++)
        ;
    return fl_head;
}

int pmm_allocs = 0; // keeping track of how many times pmm_alloc was called
int pmm_frees  = 0; // keeping track of how many times pmm_free was called

// Omar, this is a PAGE FRAME allocator no need for custom <bytes> parameter
void *pmm_alloc_page() {
    pmm_allocs++;
#ifdef PMM_DEBUG
    debugf_debug("--- Allocation n.%d ---\n", pmm_allocs);
#endif

    void *ptr = NULL;
    freelist_node *cur_node;
    for (cur_node = fl_head; cur_node != NULL; cur_node = cur_node->next) {
#ifdef PMM_DEBUG
        debugf_debug("Looking for available memory at address %p\n", cur_node);
#endif

        // if the requested size fits in the freelist region...
        if (cur_node->length >= PFRAME_SIZE) {
            // we have found a block
            break; // quit from the loop since we found a block
        }

// if not, go to the next block
#ifdef PMM_DEBUG
        debugf_debug("Not enough memory found at %p. Going on...", cur_node);
#endif
    }

    // if we've got here and nothing was found, then kernel panic
    if (cur_node == NULL) {
        kprintf_panic("OUT OF MEMORY!!\n");
        _hcf();
    }

#ifdef PMM_DEBUG
    debugf_debug("allocated %lu byte%sat address %p\n", PFRAME_SIZE,
                 PFRAME_SIZE > 1 ? "s " : " ", cur_node);
#endif

    ptr = (void *)(cur_node);

    // if memory gets allocated from the entry head, we should change it
    if (cur_node == fl_head) {
        // if there is no more space in that entry, we'll move the head to the
        // next free entry
        if ((cur_node->length - PFRAME_SIZE) <= 0) {
            fl_head = fl_head->next;
        } else { // there's still some memory left
            fl_head = (freelist_node *)((uint64_t)fl_head + PFRAME_SIZE);
            fl_head->length = cur_node->length - (uint64_t)PFRAME_SIZE;
            fl_head->next   = cur_node->next;
        }
    }

    fl_update_nodes();

#ifdef PMM_DEBUG
    debugf_debug("old head %p is now %p\n", ptr, fl_head);
    debugf_debug("\tsize: %#zx\n", fl_head->length);
    debugf_debug("\tnext: %p\n", fl_head->next);
#endif

    // zero out the whole allocated region
    memset((void *)ptr, 0, PFRAME_SIZE);

    // we need the physical address of the free entry
    return (void *)VIRT_TO_PHYSICAL(ptr);
}

void *pmm_alloc_pages(size_t pages) {
    void *ptr = pmm_alloc_page();
    for (int i = 1; i < pages; i++) {
        pmm_alloc_page();
    }

    return ptr;
}

void pmm_free(void *ptr, size_t pages) {
    pmm_frees++;
#ifdef PMM_DEBUG
    debugf_debug("--- Deallocation n.%d ---\n", pmm_frees);
#endif

    size_t s_fl_head, s_fl_ptr;

    // the entry will point to the virtual address of the deallocated
    // pointer
    freelist_node *fl_ptr = (freelist_node *)(ptr);
    fl_ptr->length        = PFRAME_SIZE * (pages > 0 ? pages : 1);

    s_fl_head = (size_t)fl_head;
    s_fl_ptr  = (size_t)fl_ptr;

#ifdef PMM_DEBUG
    debugf_debug("deallocating pointer %p\n\n", fl_ptr);
    debugf_debug("pointer %p is now a freelist entry\n", fl_ptr);
    debugf_debug("head is at location %p\n", fl_head);

#endif

    // ---------------------- //
    // --| POSSIBLE CASES |-- //
    // ---------------------- //

    // 1. the pointer comes before the head
    if (s_fl_head > s_fl_ptr) {
        fl_ptr->next = fl_head;
        // the head becomes the pointer
        fl_head      = fl_ptr;

#ifdef PMM_DEBUG
        debugf_debug("%p is the new head\n", fl_head);
#endif

        return; // deallocation is done
    }

    // 2. the pointer is somewhere in between two entries

    // we should cycle through all the entries
    for (freelist_node *fl_entry = fl_head; fl_entry != NULL;
         fl_entry                = fl_entry->next) {

        size_t s_fl_entry      = (size_t)fl_entry;
        size_t s_fl_entry_next = (size_t)fl_entry->next;

        // is the entry < pointer AND entry->next > pointer?
        if (s_fl_entry < s_fl_ptr && s_fl_entry_next > s_fl_ptr) {
            // entry->next becomes pointer->next
            fl_ptr->next = fl_entry->next;

            // entry->next becomes the pointer
            fl_entry->next = fl_ptr;

#ifdef PMM_DEBUG
            debugf_debug("%p now points to %p\n", fl_entry, fl_entry->next);
#endif

            return; // deallocation is done
        }
    }

    // 3. the pointer comes after all the entries
    freelist_node *fl_en;
    for (fl_en = fl_head; fl_en->next != NULL; fl_en = fl_en->next)
        if (s_fl_ptr > (size_t)fl_en) {
            fl_en->next = fl_ptr;

            // just in case, the entry after it will be NULL'd
            fl_en->next->next = NULL;

#ifdef PMM_DEBUG
            debugf_debug("%p is at the end of the free list\n", fl_ptr);
#endif

            return; // deallocation is done
        }
}
