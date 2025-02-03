[bits 64]

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
