symbol-file build/kernel.elf
set disassembly-flavor intel
target remote | qemu-system-x86_64 -S -gdb stdio -m 64 -cdrom kernel-v0.iso