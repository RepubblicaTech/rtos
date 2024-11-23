#include "pmm.h"

#include <limine.h>
#include <kernel.h>

#include <util/memory.h>

#include <stddef.h>
#include <stdio.h>

extern struct bootloader_data limine_parsed_data;
extern void panic();

freelist_entry *fl_head;			// is set to the first entry
freelist_entry **fl_entries_ptr;

int usable_entry_count = 0;

void pmm_init() {

	// array of freelist entries
	freelist_entry *fl_entries[limine_parsed_data.entry_count];

	// create a freelist entry that points to the start of each usable address
	for (uint64_t i = 0; i < limine_parsed_data.entry_count; i++)
	{
		struct limine_memmap_entry *memmap_entry = limine_parsed_data.memory_entries[i];
		
		if (memmap_entry->type != LIMINE_MEMMAP_USABLE) continue;

		// get virtually mapped address
		void *virtual_addr = (void*)(PHYS_TO_VIRTUAL(memmap_entry->base));
		freelist_entry *fl_entry = (freelist_entry*)virtual_addr;		// point the entry to that address

		fl_entries[usable_entry_count] = fl_entry;

		usable_entry_count++;
	}

	kprintf("[ %s():%d::INFO ] Found %d usable entries\n", __FUNCTION__, __LINE__, usable_entry_count);

	// scan the entries to point each to the next one
	for (int i = 0; i < usable_entry_count; i++)
	{
		if (fl_entries[i] == NULL) break;		// breaks the loop if there are no more entries to scan

		freelist_entry *fl_entry = fl_entries[i];

		// the next entry will be the next one in the array
		freelist_entry *fl_entry_next = fl_entries[i + 1];
		fl_entry->next = fl_entry_next;

		// if it's the first entry...
		// ...point the head to it
		if (i == 0) {
			fl_entry->prev = NULL;
			fl_head = fl_entry;
			continue;
		}

		// the prev entry will be the previous one in the array
		freelist_entry *fl_entry_prev = fl_entries[i - 1];
		fl_entry->prev = fl_entry_prev;

		// if it's the last entry, point next to NULL
		if (i == usable_entry_count - 1) {
			fl_entry->next = NULL;

			continue;
		}
	}

	// prints all entries and their links
	for (int i = 0; i < usable_entry_count; i++)
	{
		freelist_entry *fl_entry = fl_entries[i];

		kprintf("ENTRY n. %d\n", i);
		kprintf("\taddress: %p\n", fl_entry);
		kprintf("\tprev: %p\n", fl_entry->prev);
		kprintf("\tnext: %p\n", fl_entry->next);
	}

	fl_entries_ptr = fl_entries;
}

// given an array of entries, returns the count of the entries.
// @param fl_entries an array of freelist entry pointers, or a pointer to it
int get_freelist_entry_count(freelist_entry **fl_entries) {
	int fl_entry_count = 1;

	for (int i = 0; fl_entries[i] != 0x0; i++) {
		fl_entry_count++;
	}

	debugf("entry count: %d\n", fl_entry_count);

	return fl_entry_count;
}

/*
	"Refreshes" the list of entries

	@returns pointer to array of entries
*/
freelist_entry **fl_update_entries() {

	fl_entries_ptr[0] = fl_head;

	for (int i = 0; i < usable_entry_count - 1; i++)
	{
		fl_entries_ptr[i]->next = fl_entries_ptr[i + 1];

		if (i == 0) continue;
		fl_entries_ptr[i]->prev = fl_entries_ptr[i - 1];
	}

	return fl_entries_ptr;
}

int kmallocs = 0;					// keeping track of how many times kmalloc was called
int kfrees = 0;					// keeping track of how many times kfree was called

void *kmalloc(size_t bytes) {
	kmallocs++;
	debugf("--- Allocation n.%d ---\n", kmallocs);

	void *ptr;
	size_t s_fl_entry, s_fl_entry_next;

	// start with the head
	freelist_entry *fl_entry = fl_head;

	do
	{
		debugf("Looking for available memory at entry %p\n", fl_entry);

		// get current and next entry address
		s_fl_entry = (size_t)fl_entry;
		s_fl_entry_next = (size_t)fl_entry->next;

		// does <bytes> fit into the section between the two entries?
		if (bytes > (s_fl_entry_next - s_fl_entry)) {
			debugf("Not enough memory found. Looking for next address...\n");

			fl_entry = fl_entry->next;
		}

		// we have found a block
		debugf("Found enough memory at address %p\n", fl_entry);
		break;		// quit from the loop

	} while (fl_entry->next != NULL);

	// give that address to the pointer
	ptr = (void*)fl_entry;

	kprintf("[ %s():%d::INFO ] allocated %lu bytes at address %p\n", __FUNCTION__, __LINE__, bytes, fl_entry);

	// shift the entry by <bytes>
	s_fl_entry += bytes;

	debugf("head points to %p\n", fl_head);
	// if memory gets allocated from the entry head, we should change it
	if ((size_t)fl_entry == (size_t)fl_head) {
		fl_entry = (freelist_entry*)s_fl_entry;
		fl_head = fl_entry;

		debugf("head %p now points to %p\n", ptr, fl_head);
	}

	fl_update_entries();

	debugf("\tprev: %p\n", fl_head->prev);
	debugf("\tnext: %p\n", fl_head->next);

	return ptr;
}

void kfree(void *ptr) {
	kfrees++;
	debugf("--- Deallocation n.%d ---\n", kfrees);

	size_t s_fl_head, s_fl_ptr;

	freelist_entry *fl_ptr = (freelist_entry*)ptr;

	freelist_entry **fl_entries = fl_update_entries();

	fl_head = fl_entries[0];
	s_fl_head = (size_t)fl_head;
	s_fl_ptr = (size_t)fl_ptr;

	debugf("pointer %p is now a freelist entry\n", fl_ptr);
	debugf("head is at location %p\n", fl_head);

	kprintf("[ %s():%d::INFO ] deallocating pointer %p\n", __FUNCTION__, __LINE__, fl_ptr);

	// ---------------------- //
	// --| POSSIBLE CASES |-- //
	// ---------------------- //

	// 1. the pointer comes before the head
	// --- AND ---
	// 	  both pointer->next and head->next point to the same thing

	// the head becomes the pointer
	if (s_fl_head > s_fl_ptr) {
		fl_head = fl_ptr;

		// always update the list
		fl_update_entries();

		debugf("%p is the new head\n", fl_head);

		return;		// deallocation is done
	}

	// 2. the pointer is somewhere in between two entries

	// we should cycle through all the entries
	for (int i = 0; fl_entries_ptr[i] != NULL; i++)
	{
		freelist_entry *fl_entry = fl_entries_ptr[i];
		size_t s_fl_entry = (size_t)fl_entry;
		size_t s_fl_entry_next = (size_t)fl_entry->next;

		// is the entry < pointer AND entry->next > pointer?
		if (s_fl_entry < s_fl_ptr && s_fl_entry_next > s_fl_ptr) {
			// entry->next->next becomes pointer->next
			fl_ptr->next = fl_entry->next;

			// entry->next becoumes the pointer 
			fl_entry->next = fl_ptr;

			debugf("%p now points to %p\n", fl_entry, fl_entry->next);

			// always update the list
			fl_update_entries();

			return;		// deallocation is done
		}
	}
	
	// 3. the pointer comes after all the entries
	int fl_last_entry = get_freelist_entry_count(fl_entries) - 1;

	if (s_fl_ptr > (size_t)fl_entries[fl_last_entry]) {
		fl_entries[fl_last_entry + 1] = fl_ptr;

		// just in case, the entry after it will be NULL'd
		fl_entries[fl_last_entry + 2] = NULL;

		// always update the list
		fl_update_entries();

		debugf("%p is at the end of the free list\n", fl_ptr);

		return;		// deallocation is done
	}
}
