[bits 64]

; void _outb(uint16_t port, uint8_t val)
; rdi = port
; rsi = val
global _outb
_outb:
	mov dx, di
	mov ax, si

	; how it works (just so i don't have to search it again)
	; out PORT, VALUE
	out dx, al

	ret

; uint8_t _inb(uint16_t port)
; rdi = port
global _inb
_inb:
	mov dx, di

	xor ax, ax

	; how it works (just so i don't have to search it again)
	; in (some register where the value will be saved), PORT
	in al, dx

	ret

global _io_wait
_io_wait:
	push rdi
	push rsi

	mov rdi, 0x80
	mov rsi, 0x0

	call _outb

	pop rsi
	pop rdi

	ret
