#include <memory/vmm.h>

#include <memory/paging/paging.h>
#include <memory/pmm.h>
#include <stdint.h>
#include <util/string.h>

#include <mmio/mmio.h>

#include <kernel.h>

#include <stdio.h>

#include <cpu.h>

uint64_t limine_pml4;		// Limine's PML4 table
uint64_t *v_global_pml4;	// our pml4 table - virtual address
uint64_t *global_pml4;		// OUR ☭ pml4 table (will be loaded into CR4) - physical address

static struct bootloader_data limine_data;

void vmm_init() {
	limine_data = get_bootloader_data();

	limine_pml4 = _get_pml4();
	kprintf_info("Limine's PML4 sits at %llp\n", limine_pml4);

	// set up a custom PAT
	kprintf_info("PAT MSR: %#.16llx\n", _cpu_get_msr(0x277));

	uint64_t custom_pat = PAT_WRITEBACK | (PAT_WRITE_THROUGH << 8) | (PAT_WRITE_COMBINING << 16) | (PAT_UNCACHEABLE << 24);
	_cpu_set_msr(0x277, custom_pat);
	kprintf_info("Custom PAT has been set up: %#.16llx\n", _cpu_get_msr(0x277));

	v_global_pml4 = (uint64_t*)PHYS_TO_VIRTUAL(fl_alloc(PMLT_SIZE));

	// kernel addresses
	uint64_t a_kernel_start = (uint64_t)&__kernel_start;
	uint64_t a_kernel_end = (uint64_t)&__kernel_end;
	uint64_t kernel_len = a_kernel_end - a_kernel_start;

	// map the kernel file
	kprintf_info("Mapping whole kernel\n");
	map_region_to_page(v_global_pml4, a_kernel_start - VIRT_BASE + PHYS_BASE, a_kernel_start, kernel_len, PMLE_KERNEL_READ_WRITE);

	// map the whole memory
	kprintf_info("Mapping all the memory\n");
	// we will map range 0 - first entry's base
	struct limine_memmap_entry *memmap_entry = limine_parsed_data.memory_entries[0];
	map_region_to_page(v_global_pml4, 0, 0, memmap_entry->base, PMLE_KERNEL_READ_WRITE);
	map_region_to_page(v_global_pml4, 0, PHYS_TO_VIRTUAL(0), memmap_entry->base, PMLE_KERNEL_READ_WRITE);
	// maps the rest of the memory
	for (uint64_t i = 0; i < limine_parsed_data.entry_count; i++)
	{
		memmap_entry = limine_parsed_data.memory_entries[i];
		map_region_to_page(v_global_pml4, memmap_entry->base, memmap_entry->base, memmap_entry->length, PMLE_KERNEL_READ_WRITE);
		map_region_to_page(v_global_pml4, memmap_entry->base, PHYS_TO_VIRTUAL(memmap_entry->base), memmap_entry->length, PMLE_KERNEL_READ_WRITE);
	}

	//
	//	--- Mapping devices ---
	//
	mmio_device* mmio_devs = get_mmio_devices();
	for (int i = 0; i < MMIO_MAX_DEVICES; i++)
	{
		if (mmio_devs[i].base != 0x0) {
			kprintf_info("Mapping device \"%s\"\n", mmio_devs[i].name);
			map_region_to_page(v_global_pml4, mmio_devs[i].base, mmio_devs[i].base, mmio_devs[i].size, PMLE_KERNEL_READ_WRITE | PMLE_NOT_EXECUTABLE);
			map_region_to_page(v_global_pml4, mmio_devs[i].base, PHYS_TO_VIRTUAL(mmio_devs[i].base), mmio_devs[i].size, PMLE_KERNEL_READ_WRITE | PMLE_NOT_EXECUTABLE);
		}
	}

	kprintf_info("All mappings done.\n");

	kprintf_info("Our PML4 sits at %llp\n", v_global_pml4);
	global_pml4 = (uint64_t*)VIRT_TO_PHYSICAL(v_global_pml4);

	// load our page table
	kprintf_info("Loading pml4 %#llp into CR3\n", global_pml4);
	_load_pml4(global_pml4);
	kprintf_info("Guys, we're in.\n");
}
