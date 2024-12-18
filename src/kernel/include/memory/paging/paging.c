/*
	Page fault "handler"

	When a page fault exception is fired, the _hcf screen will print information about the given error code.

	(C) RepubblicaTech 2024
*/

#include "paging.h"

#include <stdio.h>
#include <stdint.h>

#include <memory/pmm.h>
#include <util/string.h>

#include <kernel.h>
#include <limine.h>

#include <cpu.h>

#include <isr.h>

/* http://wiki.osdev.org/Exceptions#Page_Fault

 31              15                             4               0
+---+--  --+---+-----+---+--  --+---+----+----+---+---+---+---+---+
|   Reserved   | SGX |   Reserved   | SS | PK | I | R | U | W | P |
+---+--  --+---+-----+---+--  --+---+----+----+---+---+---+---+---+

Bit			Name					Description
P	 	Present 					1 -> page-protection fault;
									0 -> non-present page

W	 	Write 						1 -> write access;
									0 -> read access

U	 	User 						1 -> CPL = 3. This does not necessarily mean that the page fault was a privilege violation.

R	 	Reserved write 				1 -> one or more page directory entries contain reserved bits which are set to 1. This only applies when the PSE or PAE flags in CR4 are set to 1.

I	 	Instruction Fetch 			1 -> instruction fetch. This only applies when the No-Execute bit is supported and enabled.

PK  	Protection key 				1 -> protection-key violation. The PKRU register (for user-mode accesses) or PKRS MSR (for supervisor-mode accesses) specifies the protection key rights.

SS  	Shadow stack 				1 -> shadow stack access.

SGX 	Software Guard Extensions 	1 -> SGX violation. The fault is unrelated to ordinary paging.

*/

#define PG_MASK(a)						(a & 0b1)

#define PG_PRESENT(x)					PG_MASK(x)
#define PG_WR_RD(x)						PG_MASK(x >> 1)
#define PG_RING(x)						PG_MASK(x >> 2)
#define PG_RESERVED(x)					PG_MASK(x >> 3)
#define PG_IF(x)						PG_MASK(x >> 4)
#define PG_PK(x)						PG_MASK(x >> 5)
#define PG_SS(x)						PG_MASK(x >> 6)
#define PG_SGX(x)						PG_MASK(x >> 14)

/*
	Page fault code handler

	Simply gives information about a page fault error code
	(C) RepubblicaTech 2024
*/

void pf_handler(registers* regs) {
	rsod_init();

	uint64_t pf_error_code = (uint64_t)regs->error;

	kprintf("--- PANIC! ---\n");
	kprintf("Page fault code %#016b\n\n-------------------------------\n", pf_error_code);

	kprintf(PG_RING(pf_error_code) == 0 	? "Kernel " 					: "User ");
	kprintf(PG_WR_RD(pf_error_code) == 0 	? "read attempt of a " 			: "write attempt to a ");
	kprintf(PG_PRESENT(pf_error_code) == 0 	? "non-present page entry\n\n" 	: "present page entry\n\n");

	// CR2 contains the address that caused the fault
	uint64_t cr2;
	asm volatile("mov %%cr2, %0" : "=r"(cr2));

	kprintf("RESERVED WRITE:				%d\n", PG_RESERVED(pf_error_code));
	kprintf("INSTRUCTION_FETCH:			%d\n", PG_IF(pf_error_code));
	kprintf("PROTECTION_KEY_VIOLATION:		%d\n", PG_PK(pf_error_code));
	kprintf("SHADOW_STACK_ACCESS: 			%d\n", PG_SS(pf_error_code));
	kprintf("SGX_VIOLATION: 				%d\n", PG_SGX(pf_error_code));

	kprintf("\nAttempt to access address %#llx\n\n", cr2);

	print_reg_dump(regs);

	kprintf("--- PANIC LOG END --- HALTING\n");

	_hcf();
}

/********************
 *   PAGING STUFF   *
 ********************/

uint64_t *get_create_pmlt(uint64_t *pml_table, uint64_t pmlt_index, uint64_t flags) {
	// is there something at pml_table[pmlt_index]?
	if (!(pml_table[pmlt_index] & PMLE_PRESENT)) {
		// debugf_debug("Table %llp entry %#llx is not present, creating it...\n", pml_table, pmlt_index);

		pml_table[pmlt_index] = (uint64_t)fl_alloc(PMLT_SIZE) | flags;
	}

	// kprintf_info("Table %llp entry %#llx contents:%#llx flags:%#llx\n", pml_table, pmlt_index, pml_table[pmlt_index], flags);

	return (uint64_t*)(PHYS_TO_VIRTUAL(PG_GET_ADDR(pml_table[pmlt_index])));
}

// map a page frame to a physical address that gets mapped to a virtual one
void map_phys_to_page(uint64_t* pml4_table, uint64_t physical, uint64_t virtual, uint64_t flags) {
	if (virtual % PMLT_SIZE) {
		kprintf_panic("Attempted to map non-aligned addresses (phys)%#llx (virt)%#llx! Halting...\n", physical, virtual);
		_hcf();
	}
	
	uint64_t pml4_index = PML4_INDEX(virtual);
	uint64_t pdp_index	= PDP_INDEX(virtual);
	uint64_t pdir_index = PDIR_INDEX(virtual);
	uint64_t ptab_index = PTAB_INDEX(virtual);

	uint64_t *pdp_table = get_create_pmlt(pml4_table, pml4_index, flags);
	uint64_t *pdir_table = get_create_pmlt(pdp_table, pdp_index, flags);
	uint64_t *page_table = get_create_pmlt(pdir_table, pdir_index, flags);

	page_table[ptab_index] = PG_PHYS_ADDR(physical) | flags;
	// debugf_debug("Page table %llp entry %llu mapped to (virt)%#llx (phys)%#llx \n", page_table, ptab_index, virtual, physical);
	// debugf_debug("\tcontents of table[entry]: %#llx\n", page_table[ptab_index]);

	_invalidate(virtual);
}

// maps a page region to its physical range
void map_region_to_page(uint64_t* pml4_table, uint64_t physical_start, uint64_t virtual_start, uint64_t len, uint64_t flags) {

	uint64_t pages = ALIGN_UP(len / PMLT_SIZE, PMLT_SIZE);

	kprintf_info("Mapping address range (phys)%#llx-%#llx (virt)%#llx-%#llx\n", physical_start, physical_start + len, virtual_start, virtual_start + len);

	for (uint64_t i = 0; i < pages; i ++)
	{
		uint64_t phys = physical_start + (i * PMLT_SIZE);
		uint64_t virt = virtual_start + (i * PMLT_SIZE);
		map_phys_to_page(pml4_table, phys, virt, flags);
	}

}
