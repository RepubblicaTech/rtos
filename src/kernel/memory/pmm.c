/*
        Freelist memory allocator/manager

        (C) RepubblicaTech 2024
*/

#include "pmm.h"

#include <kernel.h>
#include <limine.h>

#include <util/string.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "paging/paging.h"

static struct bootloader_data limine_data;
extern void _hcf();

freelist_node *fl_head; // is set to the first entry

freelist_node **fl_entries_ptr;

int usable_entry_count = 0;

void pmm_init() {

    limine_data = get_bootloader_data();

    // array of freelist entries
    freelist_node *fl_entries[limine_data.usable_entry_count];

    // create a freelist entry that points to the start of each usable address
    for (uint64_t i = 0; i < limine_data.memmap_entry_count; i++) {
        struct limine_memmap_entry *memmap_entry =
            limine_data.limine_memory_map[i];

        if (memmap_entry->type != LIMINE_MEMMAP_USABLE)
            continue;

        // get virtual address
        void *virtual_addr = (void *)(PHYS_TO_VIRTUAL(memmap_entry->base));
        freelist_node *fl_entry =
            (freelist_node *)virtual_addr; // point the entry to that address
        fl_entry->length = ((size_t)memmap_entry->length);

        fl_entries[usable_entry_count] = fl_entry;

        usable_entry_count++;
    }

    fl_entries[usable_entry_count] = NULL;

    debugf_debug("Found %d usable ranges\n", usable_entry_count);

    // scan the entries to point each to the next one
    for (int i = 0; fl_entries[i] != NULL; i++) {
        freelist_node *fl_entry = fl_entries[i];

        // the next entry will be the next one in the array
        freelist_node *fl_entry_next = fl_entries[i + 1];
        fl_entry->next               = fl_entry_next;

        // if it's the first entry...
        // ...point the head to it
        if (i == 0) {
            fl_entry->prev = NULL;
            fl_head        = fl_entry;
            continue;
        }

        // the prev entry will be the previous one in the array
        freelist_node *fl_entry_prev = fl_entries[i - 1];
        fl_entry->prev               = fl_entry_prev;

        // if it's the last entry, point next to NULL
        if (i == usable_entry_count - 1) {
            fl_entry->next = NULL;

            continue;
        }
    }

    // prints all entries and their links
    for (int i = 0; i < usable_entry_count; i++) {
        freelist_node *fl_entry = fl_entries[i];

        debugf_debug("ENTRY n. %d\n", i);
        debugf_debug("address: %p\n", fl_entry);
        debugf_debug("\tprev: %p\n", fl_entry->prev);
        debugf_debug("\tnext: %p\n", fl_entry->next);
        debugf_debug("\tsize: %#llx\n", fl_entry->length);
    }

    fl_entries_ptr = fl_entries;
}

// Returns the count of the entries.
int get_freelist_entry_count() {
    freelist_node *fl_en = fl_head;
    for (usable_entry_count = 0; fl_en != NULL;
         usable_entry_count++, fl_en = fl_en->next)
        ;

    debugf_debug("entry count: %d\n", usable_entry_count);

    return usable_entry_count;
}

/*
        "Refreshes" the list of entries

        @returns pointer to array of entries
*/
freelist_node **fl_update_entries() {

    for (freelist_node *fl_en = fl_head; fl_en != NULL; fl_en = fl_en->next) {
        fl_entries_ptr[usable_entry_count] = fl_en;
    }

    return fl_entries_ptr;
}

<<<<<<< HEAD
int kmallocs = 0;					// keeping track of how many times pmm_alloc was called
int kfrees = 0;						// keeping track of how many times pmm_free was called

void *pmm_alloc(size_t bytes) {
	if (bytes < 1) {
		debugf_debug("Bro are you ok with %lu bytes?\n", bytes);
		return NULL;
	}
=======
int kmallocs = 0; // keeping track of how many times fl_alloc was called
int kfrees   = 0; // keeping track of how many times fl_free was called

void *fl_alloc(size_t bytes) {
    if (bytes < 1) {
        debugf_debug("Bro are you ok with %lu bytes?\n", bytes);
        return NULL;
    }
>>>>>>> 1ed5a09bf09a1b3305b7b4c24af03169c276b820

    kmallocs++;
#ifdef PMM_DEBUG
    debugf_debug("--- Allocation n.%d ---\n", kmallocs);
#endif

    void *ptr;

    // start with the head
    freelist_node *fl_entry;

    for (fl_entry = fl_head; fl_entry != NULL; fl_entry = fl_entry->next) {
#ifdef PMM_DEBUG
        debugf_debug("Looking for memory to allocate at address %p\n",
                     fl_entry);
#endif

        // if the requested size fits in the freelist region...
        if (bytes <= fl_entry->length) {
            // we have found a block
            fl_entry->length -=
                bytes; // subract the allocated size from entry's length

            break; // quit from the loop since we found a block
        }
// if not, go to the next block
#ifdef PMM_DEBUG
        debugf_debug(
            "Not enough memory found at %p. Looking for next range %p...\n",
            fl_entry, fl_entry->next);
#endif
    }
    // if we've got here and nothing was found, then kernel panic
    if (fl_entry == NULL) {
        kprintf_panic("OUT OF MEMORY!!\n");
        _hcf();
    }

#ifdef PMM_DEBUG
    debugf_debug("allocated %lu byte%sat address %p\n", bytes,
                 bytes > 1 ? "s " : " ", fl_entry);
#endif

    // if memory gets allocated from the entry head, we should change it
    if ((size_t)fl_entry == (size_t)fl_head) {
        // if there are no more bytes in that entry, we'll move the head to the
        // next free entry
        if (fl_entry->length == 0) {
            fl_head = fl_head->next;
        } else { // there's still some memory left
            fl_head         = (freelist_node *)((size_t)fl_entry + bytes);
            fl_head->next   = fl_entry->next;
            fl_head->length = fl_entry->length;
        }
    }

    ptr = (void *)((size_t)fl_entry);

    // freelist_node **fl_entries = fl_update_entries();

#ifdef PMM_DEBUG
    debugf_debug("old head %p is now %p\n", ptr, fl_head);
    debugf_debug("\tprev: %p\n", fl_head->prev);
    debugf_debug("\tnext: %p\n", fl_head->next);
    debugf_debug("\tsize: %#llx\n", fl_head->length);
#endif

    // zero out the whole allocated region
    memset(ptr, 0, bytes);

    // we need the physical address of the free entry
    return (void *)VIRT_TO_PHYSICAL(ptr);
}

<<<<<<< HEAD
void pmm_free(void *ptr) {
	kfrees++;
	#ifdef PMM_DEBUG
		debugf_debug("--- Deallocation n.%d ---\n", kfrees);
	#endif
=======
void fl_free(void *ptr) {
    kfrees++;
#ifdef PMM_DEBUG
    debugf_debug("--- Deallocation n.%d ---\n", kfrees);
#endif
>>>>>>> 1ed5a09bf09a1b3305b7b4c24af03169c276b820

    size_t s_fl_head, s_fl_ptr;

    // the entry will point to the virtual address of the deallocated pointer
    freelist_node *fl_ptr = (freelist_node *)(ptr + limine_data.hhdm_offset);

    s_fl_head = (size_t)fl_head;
    s_fl_ptr  = (size_t)fl_ptr;

#ifdef PMM_DEBUG
    debugf_debug("pointer %p is now a freelist entry\n", fl_ptr);
    debugf_debug("head is at location %p\n", fl_head);

    debugf_debug("deallocating pointer %p\n", fl_ptr);
#endif

    // ---------------------- //
    // --| POSSIBLE CASES |-- //
    // ---------------------- //

    // 1. the pointer comes before the head
    if (s_fl_head > s_fl_ptr) {
        // the head becomes the pointer
        fl_head = fl_ptr;

#ifdef PMM_DEBUG
        debugf_debug("%p is the new head\n", fl_head);
#endif

        return; // deallocation is done
    }

    // 2. the pointer is somewhere in between two entries

    // we should cycle through all the entries
    for (int i = 0; fl_entries_ptr[i] != NULL; i++) {
        freelist_node *fl_entry = fl_entries_ptr[i];
        size_t s_fl_entry       = (size_t)fl_entry;
        size_t s_fl_entry_next  = (size_t)fl_entry->next;

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
    freelist_node *last_fl;
    freelist_node *fl_en;
    for (fl_en = fl_head; fl_en->next != NULL; fl_en = fl_en->next)
        last_fl = fl_en;

    if (s_fl_ptr > (size_t)last_fl) {
        last_fl->next = fl_ptr;

        // just in case, the entry after it will be NULL'd
        last_fl->next->next = NULL;

        // update the entries (?)
        fl_en->prev->next = last_fl;
        last_fl->prev     = fl_en->prev;

#ifdef PMM_DEBUG
        debugf_debug("%p is at the end of the free list\n", fl_ptr);
#endif

        return; // deallocation is done
    }
}
