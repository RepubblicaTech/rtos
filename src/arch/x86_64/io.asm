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

; void _outw(uint16_t port, uint16_t val)
global _outw
_outw:
    mov dx, di
    mov ax, si

    out dx, ax

    ret

; void _outd(uint16_t port, uint32_t val)
global _outd
_outd:
    mov dx, di
    mov ax, si

    out dx, eax

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

; uint16_t _inw(uint16_t port)
global _inw
_inw:
    mov dx, di
    xor ax, ax

    in ax, dx

    ret

; uint32_t _ind(uint16_t port)
global _ind
_ind:
    mov dx, di
    xor ax, ax

    in eax, dx

    ret

; void _io_wait(void)
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
