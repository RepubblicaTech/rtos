#include "vmm.h"

#include <memory/paging/paging.h>
#include <memory/pmm.h>
#include <util/memory.h>

#include <kernel.h>

#include <stdio.h>

uint64_t *limine_pml4;		// Limine's PML4 table
uint64_t *v_global_pml4;	// our pml4 table - virtual address
uint64_t *global_pml4;		// OUR ☭ pml4 table (will be loaded into CR4) - physical address

void vmm_init() {
	
	limine_pml4 = get_pml4();
	kprintf_info("Limine's PML4 sits at %llp\n", limine_pml4);

	v_global_pml4 = (uint64_t*)PHYS_TO_VIRTUAL(fl_alloc(PMLT_SIZE));

	// kernel
	uint64_t a_kernel_start = (uint64_t)&__kernel_start;
	uint64_t a_kernel_end = (uint64_t)&__kernel_end;
	uint64_t kernel_len = a_kernel_end - a_kernel_start;

	// map the kernel file
	kprintf_info("Mapping whole kernel\n");
	map_region_to_page(v_global_pml4, a_kernel_start - VIRT_BASE + PHYS_BASE, a_kernel_start, kernel_len, PMLE_KERNEL_READ_WRITE);

	//
	//	--- MAPPING DEVICES ---
	//

	// map the whole memory
	kprintf_info("Mapping all the memory\n");
	for (uint64_t i = 0; i < limine_parsed_data.entry_count; i++)
	{
		struct limine_memmap_entry *memmap_entry = limine_parsed_data.memory_entries[i];
		map_region_to_page(v_global_pml4, memmap_entry->base, memmap_entry->base, memmap_entry->length, PMLE_KERNEL_READ_WRITE);
		map_region_to_page(v_global_pml4, memmap_entry->base, PHYS_TO_VIRTUAL(memmap_entry->base), memmap_entry->length, PMLE_KERNEL_READ_WRITE);
	}
	
	kprintf_info("All mappings done.\n");

	kprintf_info("Our PML4 sits at %llp\n", v_global_pml4);
	global_pml4 = (uint64_t*)VIRT_TO_PHYSICAL(v_global_pml4);
	
	// load our page table
	kprintf_info("Loading pml4 %#llp into CR3\n", global_pml4);
	_load_pml4(global_pml4);
	kprintf_info("Guys, we're in.\n");
}
