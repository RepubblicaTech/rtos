#include "page.h"

#include <stdio.h>

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

#define PRESENT(x)					(x & 0x01)
#define WR_RD(x)					((x >> 1) & 0x01)
#define RING(x)						((x >> 2) & 0x01)
#define RESERVED(x)					((x >> 3) & 0x01)
#define IF(x)						((x >> 4) & 0x01)
#define PK(x)						((x >> 5) & 0x01)
#define SS(x)						((x >> 6) & 0x01)
#define SGX(x)						((x >> 14) & 0x01)

void pf_handler(registers* regs) {
	rsod_init();

	uint64_t pf_error_code = (uint64_t)regs->error;

	kprintf("--- PANIC! ---\n");
	kprintf("Page fault code %#06lx\n\n-------------------------------\n", pf_error_code);

	kprintf("SGX_VIOLATION: %d\n", SGX(pf_error_code));
	kprintf("SHADOW_STACK_ACCESS: %d\n", SS(pf_error_code));
	kprintf("PROTECTION_KEY_VIOLATION: %d\n", PK(pf_error_code));
	kprintf("INSTRUCTION_FETCH: %d\n", IF(pf_error_code));
	kprintf("RESERVED WRITE: %d\n", RESERVED(pf_error_code));

	kprintf("CPL: %d\n", RING(pf_error_code) == 0 ? 0 : 3);
	kprintf("READ_WRITE_ACCESS: %d\n", WR_RD(pf_error_code));
	kprintf("PRESENT: %d\n-------------------------------\n", PRESENT(pf_error_code));

	kprintf("Instruction address: %#lx\n", regs->rip);

	kprintf("--- PANIC LOG END --- HALTING\n");

	panic();
}
