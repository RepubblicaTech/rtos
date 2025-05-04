[bits 64]

; uint64_t get_tsc()
global _get_tsc
_get_tsc:

	rdtsc

	mov rax, rdx
	shl rax, 32
	or rax, rdx

	ret
