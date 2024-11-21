symbol-file build/kernel.elf
set disassembly-flavor intel
target remote | qemu-system-x86_64 -S -gdb stdio -cdrom kernel-v0.iso