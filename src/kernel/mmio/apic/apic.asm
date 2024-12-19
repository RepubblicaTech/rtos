[bits 64]

; i don't know how to 'bts' in C

; check arch/cpu.asm for these functions
extern _cpu_get_msr					; extern uint64_t _cpu_get_msr(uint32_t msr)
extern _cpu_set_msr					; extern void _cpu_set_msr(uint32_t msr, uint64_t value)

; uint64_t apic_global_enable()
global _apic_global_enable
_apic_global_enable:

	push rdi
	xor rdi, rdi

	mov edi, 0x1b
	call _cpu_get_msr

	; rax = our MSR value
	bts rax, 11

	; write it back to the MSR to enable the APIC
	push rsi
	xor rsi, rsi
	xor rdi, rdi
	mov edi, 0x1b
	mov rsi, rax
	call _cpu_set_msr

	pop rsi
	pop rdi

	; i'll also return EAX just in case :)
	ret

