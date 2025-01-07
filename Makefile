TARGET_BASE=x86_64
TARGET=$(TARGET_BASE)-elf
TOOLCHAIN_PREFIX=$(abspath toolchain/$(TARGET))
export PATH:=$(TOOLCHAIN_PREFIX)/bin:$(PATH)

OS_CODENAME=kernel-v0

PATCHES_DIR=patches
EXTERN_LIBS_DIR=thirdparty
BUILD_DIR=build
ISO_DIR=iso
OBJS_DIR=$(BUILD_DIR)/objs

# Nuke built-in rules and variables.
override MAKEFLAGS += -rR

# This is the name that our final kernel executable will have.
# Change as needed.
override KERNEL := kernel.elf

# Convenience macro to reliably declare user overridable variables.
define DEFAULT_VAR =
    ifeq ($(origin $1),default)
        override $(1) := $(2)
    endif
    ifeq ($(origin $1),undefined)
        override $(1) := $(2)
    endif
endef

# It is suggested to use a custom built cross toolchain to build a kernel.
# We are using the standard "cc" here, it may work by using
# the host system's toolchain, but this is not guaranteed.
override DEFAULT_KCC := $(TARGET)-gcc
$(eval $(call DEFAULT_VAR,KCC,$(DEFAULT_KCC)))

# Same thing for "ld" (the linker).
override DEFAULT_KLD := $(TARGET)-ld
$(eval $(call DEFAULT_VAR,KLD,$(DEFAULT_KLD)))

# User controllable C flags.
override DEFAULT_KCFLAGS := -g -O0 -pipe
$(eval $(call DEFAULT_VAR,KCFLAGS,$(DEFAULT_KCFLAGS)))

# User controllable C preprocessor flags. We set none by default.
override DEFAULT_KCPPFLAGS :=
$(eval $(call DEFAULT_VAR,KCPPFLAGS,$(DEFAULT_KCPPFLAGS)))

# User controllable nasm flags.
override DEFAULT_KNASMFLAGS := -F dwarf -g
$(eval $(call DEFAULT_VAR,KNASMFLAGS,$(DEFAULT_KNASMFLAGS)))

# User controllable linker flags. We set none by default.
override DEFAULT_KLDFLAGS :=
$(eval $(call DEFAULT_VAR,KLDFLAGS,$(DEFAULT_KLDFLAGS)))

# Internal C flags that should not be changed by the user.
override KCFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -I src/kernel/include \
	-I src/arch/$(TARGET_BASE)/include \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-PIC \
    -m64 \
    -march=x86-64 \
    -mno-80387 \
    -mno-mmx \
	-mno-sse \
	-mno-sse2 \
    -mno-red-zone \
    -mcmodel=kernel \

# Internal C preprocessor flags that should not be changed by the user.
override KCPPFLAGS := \
    $(KCPPFLAGS) \
    -MMD \
    -MP

# Internal linker flags that should not be changed by the user.
override KLDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    -T src/linker.ld \
	-Map=$(BUILD_DIR)/kernel.map

# Internal nasm flags that should not be changed by the user.
override KNASMFLAGS += \
    -Wall \
    -f elf64

# Use "find" to glob all *.c, *.S, and *.asm files in the tree and obtain the
# object and header dependency file names.
override CFILES := $(shell cd src && find -L * -type f -name '*.c')
override ASFILES := $(shell cd src && find -L * -type f -name '*.S')
override NASMFILES := $(shell cd src && find -L * -type f -name '*.asm')
override OBJ := $(addprefix $(OBJS_DIR)/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o) $(NASMFILES:.asm=.asm.o))
override HEADER_DEPS := $(addprefix $(OBJS_DIR)/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

# Default target.
.PHONY: all limine_build toolchain

all: clean $(OS_CODENAME).iso

$(OS_CODENAME).iso: bootloader
	@# Create the bootable ISO.
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
	        -no-emul-boot -boot-load-size 4 -boot-info-table \
	        --efi-boot boot/limine/limine-uefi-cd.bin \
	        -efi-boot-part --efi-boot-image --protective-msdos-label \
	        $(ISO_DIR) -o $(OS_CODENAME).iso

	@# Install Limine stage 1 and 2 for legacy BIOS boot.
	./$(EXTERN_LIBS_DIR)/limine/limine bios-install $(OS_CODENAME).iso

bootloader: deps $(BUILD_DIR)/$(KERNEL)
	mkdir -p $(ISO_DIR)
	@# Copy the relevant files over.
	mkdir -p $(ISO_DIR)/boot
	cp -v $(BUILD_DIR)/$(KERNEL) $(ISO_DIR)/boot/
	mkdir -p $(ISO_DIR)/boot/limine
	cp -v src/limine.conf $(EXTERN_LIBS_DIR)/limine/limine-bios.sys $(EXTERN_LIBS_DIR)/limine/limine-bios-cd.bin \
	      $(EXTERN_LIBS_DIR)/limine/limine-uefi-cd.bin $(ISO_DIR)/boot/limine/

	@# Create the EFI boot tree and copy Limine's EFI executables over.
	mkdir -p $(ISO_DIR)/EFI/BOOT
	cp -v $(EXTERN_LIBS_DIR)/limine/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	cp -v $(EXTERN_LIBS_DIR)/limine/BOOTIA32.EFI $(ISO_DIR)/EFI/BOOT/

deps: limine_build $(EXTERN_LIBS_DIR)/flanterm $(EXTERN_LIBS_DIR)/nanoprintf
	cp -vf $(EXTERN_LIBS_DIR)/limine/limine.h src/kernel/include/limine.h

	@# copy flanterm headers
	mkdir -p src/kernel/include/flanterm/backends
	mkdir -p src/kernel/flanterm/backends
	cp -vf $(EXTERN_LIBS_DIR)/flanterm/*.h src/kernel/include/flanterm
	cp -vf $(EXTERN_LIBS_DIR)/flanterm/*.c src/kernel/flanterm

	cp -vf $(EXTERN_LIBS_DIR)/flanterm/backends/*.h src/kernel/include/flanterm/backends
	cp -vf $(EXTERN_LIBS_DIR)/flanterm/backends/*.c src/kernel/flanterm/backends

	@# custom font header
	cp -vf $(PATCHES_DIR)/font.h src/kernel/include/flanterm/backends

	@# due to the project structure, the include paths have to be modified
	patch -u src/kernel/flanterm/flanterm.c -i $(PATCHES_DIR)/flanterm.c.patch
	patch -u src/kernel/flanterm/backends/fb.c -i $(PATCHES_DIR)/fb.c.patch

	cp -vf $(EXTERN_LIBS_DIR)/nanoprintf/nanoprintf.h src/kernel/include

limine_build: update_submodules
	@# Build "limine" utility
	make -C $(EXTERN_LIBS_DIR)/limine

update_submodules: $(EXTERN_LIBS_DIR)/limine $(EXTERN_LIBS_DIR)/flanterm $(EXTERN_LIBS_DIR)/nanoprintf
	git submodule update --recursive --remote

# Link rules for the final kernel executable.
$(BUILD_DIR)/$(KERNEL): Makefile src/linker.ld $(OBJ) always
	mkdir -p "$$(dirname $@)"
	$(KLD) $(OBJ) $(KLDFLAGS) -o $@
	@echo "--> Built:	" $@

# Include header dependencies.
-include $(HEADER_DEPS)

# Compilation rules for *.c files.
$(OBJS_DIR)/%.c.o: src/%.c Makefile always
	mkdir -p "$$(dirname $@)"
	$(KCC) $(KCFLAGS) $(KCPPFLAGS) -c $< -o $@
	@echo "--> Compiled:	" $<

# Compilation rules for *.S files.
$(OBJS_DIR)/%.S.o: src/%.S Makefile always
	mkdir -p "$$(dirname $@)"
	$(KCC) $(KCFLAGS) $(KCPPFLAGS) -c $< -o $@
	@echo "--> Compiled:	" $<

# Compilation rules for *.asm (nasm) files.
$(OBJS_DIR)/%.asm.o: src/%.asm Makefile always
	mkdir -p "$$(dirname $@)"
	nasm $(KNASMFLAGS) $< -o $@
	@echo "--> Compiled:	" $<

run: $(OS_CODENAME).iso
	qemu-system-x86_64 \
		-m 64M \
		-debugcon stdio \
		-cdrom $<

debug: $(OS_CODENAME).iso
	gdb -x debug.gdb $(BUILD_DIR)/$(KERNEL)

# Remove object files and the final executable.
.PHONY: clean always

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(ISO_DIR)

clean-all: clean
	rm -rf limine
	rm -rf flanterm
	rm -rf nanoprintf

always: update_submodules
	mkdir -p $(BUILD_DIR)
	mkdir -p $(OBJS_DIR)
	mkdir -p $(ISO_DIR)
