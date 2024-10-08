TARGET_BASE=x86_64
TARGET=$(TARGET_BASE)-elf
TOOLCHAIN_PREFIX=$(abspath toolchain/$(TARGET))
export PATH:=$(TOOLCHAIN_PREFIX)/bin:$(PATH)

OS_CODENAME=kernel-v0

BUILD_DIR=build
ISO_DIR=iso
OBJS_DIR=$(BUILD_DIR)/objs

# Nuke built-in rules and variables.
override MAKEFLAGS += -rR

# This is the name that our final kernel executable will have.
# Change as needed.
override KERNEL := kernel

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
override DEFAULT_KCFLAGS := -g -O2 -pipe
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
	-g \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -I src/include \
	-I src/arch/$(TARGET_BASE) \
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
    -T src/linker.ld

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
	./limine/limine bios-install $(OS_CODENAME).iso

bootloader: limine_build $(BUILD_DIR)/$(KERNEL)
	mkdir -p $(ISO_DIR)
	@# Copy the relevant files over.
	mkdir -p $(ISO_DIR)/boot
	cp -v $(BUILD_DIR)/$(KERNEL) $(ISO_DIR)/boot/
	mkdir -p $(ISO_DIR)/boot/limine
	cp -v src/limine.conf limine/limine-bios.sys limine/limine-bios-cd.bin \
	      limine/limine-uefi-cd.bin $(ISO_DIR)/boot/limine/

	@# Create the EFI boot tree and copy Limine's EFI executables over.
	mkdir -p $(ISO_DIR)/EFI/BOOT
	cp -v limine/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI $(ISO_DIR)/EFI/BOOT/

limine_build: limine
	@# Build "limine" utility
	make -C limine
	@# Always update limine.h in case of updates
	cp -vf limine/limine.h src/include/limine.h

limine:
	@# Download the latest Limine binary release for the 8.x branch
	git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1

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
		-m 64 \
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

always: update_limine
	mkdir -p $(BUILD_DIR)
	mkdir -p $(OBJS_DIR)
	mkdir -p $(ISO_DIR)

update_limine: limine
	cd $< && git pull
