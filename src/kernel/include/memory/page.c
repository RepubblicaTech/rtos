#include "page.h"

#include <stdio.h>

/* http://wiki.osdev.org/Exceptions#Page_Fault

 31              15                             4               0
+---+--  --+---+-----+---+--  --+---+----+----+---+---+---+---+---+
|   Reserved   | SGX |   Reserved   | SS | PK | I | R | U | W | P |
+---+--  --+---+-----+---+--  --+---+----+----+---+---+---+---+---+

Bit			Name					Description
P	 	Present 					1 -> page-protection violation;
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

void pf_handler(registers* regs) {
	rsod_init();

	uint64_t pf_error_code = (uint64_t)regs->error;

	kprintf("--- PANIC! ---\n");

	switch (pf_error_code)
	{
		case 0b000:
			kprintf("Kernel read attempt on a non-present page entry\n");
			break;
		case 0b010:
			kprintf("Kernel write attempt on a non-present page entry\n");
			break;
		case 0b001:
			kprintf("Kernel protection fault on read attempt\n");
			break;
		case 0b011:
			kprintf("Kernel protection fault on write access\n");
			break;
		case 0b100:
			kprintf("User read attempt on a non-present page entry\n");
			break;
		case 0b110:
			kprintf("User write attempt on a non-present page entry\n");
			break;
		case 0b101:
			kprintf("User protection fault on read attempt\n");
			break;
		case 0b111:
			kprintf("User protection fault on write access\n");
			break;

		default:
			kprintf("Unknown Error\n");
			break;
	}

	kprintf("--- PANIC LOG END --- HALTING\n");

	panic();
}
