/*
	- Page fault handler

	- Paging utilities
*/

#include <isr.h>

void PMLE_handler(registers* regs);

void paging_init();

/*
	--- Some definitions ---

	PMLT: Page Map Level Table. Refers to a generic page table in any level	
								(e.g. PDPT, PDT, PT)

	PMLE: Page Map Level Entry. Refers to an entry in a PMLT
								(e.g. PDPE, PDE, PE)

	PG: suffix that refers to paging-related macros and constants
*/

#define VIRT_BASE limine_parsed_data.kernel_base_virtual
#define PHYS_BASE limine_parsed_data.kernel_base_physical

// PMLT entry is 8-bytes wide instead of 4, so the entries are halved
#define PMLT_SIZE 		0x1000		// each page frame is 4KB wide
#define PMLT_ENTRIES	512

// PMLE Flags
// from https://github.com/tyler-ottman/tellurium-os/blob/01cf40ba4ab872e81bc98d6c740174c42b029376/kernel/memory/vmm.h#L15
#define PMLE_PRESENT             1
#define PMLE_WRITE               (1 << 1)
#define PMLE_USER                (1 << 2)
#define PMLE_PWT                 (1 << 3)
#define PMLE_PCD                 (1 << 4)
#define PMLE_ACCESSED            (1 << 5)
#define PMLE_NOT_EXECUTABLE      0x8000000000000000

// Page privileges attributes
// from https://github.com/Tix3Dev/apoptOS/blob/370fd34a6d3c87a9d1a16d1a2ec072bd1836ba6c/src/kernel/memory/virtual/vmm.h#L39
#define PMLE_KERNEL_READ			PMLE_PRESENT
#define PMLE_KERNEL_READ_WRITE		PMLE_PRESENT | PMLE_WRITE
#define PMLE_USER_READ				PMLE_PRESENT | PMLE_USER
#define PMLE_USER_READ_WRITE		PMLE_PRESENT | PMLE_WRITE | PMLE_USER

// by https://github.com/malwarepad/cavOS/blob/646237dfd6c4173a4c059cccd74c63dd31cfd052/src/kernel/include/paging.h

// physical address offset mask
#define PG_PHYSICAL_MASK	0x0000fffffffff000
						  // e.g.
						  //0xFFFF80000005208a
						  //0x0000800000052000	(masked)

#define PG_GET_ADDR(a) 		((a) & PG_PHYSICAL_MASK)
#define PG_FLAGS(a) 		((a) & ~(PG_PHYSICAL_MASK))
#define PG_PHYS_ADDR(x) 	((x) & ~0xFFF)

#define PMLT_MASK			0x1ff
#define PG_OFFSET_MAK		0x3ff

// virtual address macros to get the PMLEs
#define PML4_INDEX(a)		((a >> 39) & PMLT_MASK)
#define PDP_INDEX(a)		((a >> 30) & PMLT_MASK)
#define PDIR_INDEX(a)		((a >> 21) & PMLT_MASK)
#define PTAB_INDEX(a)		((a >> 12) & PMLT_MASK)

// Paging alignment macros
// from https://github.com/Tix3Dev/apoptOS/blob/370fd34a6d3c87a9d1a16d1a2ec072bd1836ba6c/src/kernel/memory/mem.h#L36
#define ALIGN_DOWN(address, align)  ((address) & ~((align)-1))
#define ALIGN_UP(address, align)    (((address) + (align)-1) & ~((align)-1))

void pf_handler(registers* regs);

/********************
 *   PAGING STUFF   *
 ********************/

uint64_t *get_pml4();

// sets the PML4 table to use
// note: given address MUST be physical, NOT a virtual one
extern void _load_pml4(uint64_t *pml4_base);

// flushes a virtual address in the TLB
extern void _invalidate(uint64_t virtual);

uint64_t *get_create_pmlt(uint64_t *pml_table, uint64_t pmlt_index, uint64_t flags);

void map_phys_to_page(uint64_t* pml4_table, uint64_t physical, uint64_t virtual, uint64_t flags);
void map_region_to_page(uint64_t* pml4_table, uint64_t physical_start, uint64_t virtual_start, uint64_t len, uint64_t flags);
