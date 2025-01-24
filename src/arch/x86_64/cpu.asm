[bits 64]

; void* get_regs()
; just returns the address of rsp with all the needed registers pushed to the stack
; only time we're going to use it is to initialize the scheduletr :^)
global get_regs
get_regs:
    ; push all registers

    ; nothing for interrupt number and errorcode
    push 0
    push 0

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; push ds
    mov rbp, ds
    push rbp

    mov bx, 0x10
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    mov ss, bx

    mov rax, rsp
    ret

; uint64_t _cpu_get_msr(uint32_t msr)
global _cpu_get_msr
_cpu_get_msr:
    ; first argument's lower 32 bits (-> rdi & 0x0000FFFF)
    mov ecx, edi

    ; reset rax and rdx values
    xor rax, rax
    xor rdx, rdx

    ; ecx = requested MSR
    rdmsr
    ; rax = lower 32 bits of the result
    ; rdx = higher 32 bits

    ; rax |= (rdx << 32)
    shl rdx, 32
    or rax, rdx

    ret

; _cpu_set_msr(uint32_t msr, uint64_t value)
global _cpu_set_msr
_cpu_set_msr:
    ; first argument (MSR)
    mov ecx, edi

    xor rax, rax
    xor rdx, rdx

    ; eax = second argument's lower 32 bits (esi)
    mov rax, rsi

    ; rdx >> 32
    mov rdx, rax
    shr rdx, 32

    ; ecx = requested MSR
    ; eax = lower 32-bits of the value
    ; edx = higher 32-bits
    wrmsr

    ret
