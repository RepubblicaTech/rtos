[bits 64]

global _load_gdt
_load_gdt:
    lgdt [rdi]
    ret
