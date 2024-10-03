[bits 64]

global load_gdt
load_gdt:
    lgdt [rdi]
    ret
