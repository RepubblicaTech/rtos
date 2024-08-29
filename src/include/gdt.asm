[bits 64]

global load_gdt
load_gdt:
    lgdt [rdi]

    ret

global GDT_CODE_SEGMENT
global GDT_DATA_SEGMENT

GDT_CODE_SEGMENT dw 8h
GDT_DATA_SEGMENT dw 10h
