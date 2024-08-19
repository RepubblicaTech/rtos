TARGET=x86_64-elf
TOOLCHAIN_PREFIX = $(abspath $(TARGET))
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)

CPU_CORES=8

export BUILD_DIR = $(abspath build)

.PHONY:  toolchain toolchain_binutils toolchain_gcc clean-toolchain clean-toolchain-all

BINUTILS_VERSION = 2.41
BINUTILS_URL = https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz

GCC_VERSION = 14.2.0
GCC_URL = https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz

toolchain: toolchain_binutils toolchain_gcc

BINUTILS_SRC = binutils-$(BINUTILS_VERSION)
BINUTILS_BUILD = binutils-build-$(BINUTILS_VERSION)

toolchain_binutils: $(TOOLCHAIN_PREFIX)/bin/i686-elf-ld

$(TOOLCHAIN_PREFIX)/bin/i686-elf-ld: $(BINUTILS_SRC).tar.xz
	tar -xf binutils-$(BINUTILS_VERSION).tar.xz
	mkdir $(BINUTILS_BUILD)
	cd $(BINUTILS_BUILD) && CFLAGS= ASMFLAGS= CC= CXX= LD= ASM= LINKFLAGS= LIBS= ../binutils-$(BINUTILS_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)"	\
		--target=$(TARGET)				\
		--with-sysroot					\
		--disable-nls					\
		--disable-werror
	make -j$(CPU_CORES) -C $(BINUTILS_BUILD)
	make -C $(BINUTILS_BUILD) install

$(BINUTILS_SRC).tar.xz:
	wget $(BINUTILS_URL)

GCC_SRC = gcc-$(GCC_VERSION)
GCC_BUILD = gcc-build-$(GCC_VERSION)

toolchain_gcc: $(TOOLCHAIN_PREFIX)/bin/i686-elf-gcc

$(TOOLCHAIN_PREFIX)/bin/i686-elf-gcc: $(TOOLCHAIN_PREFIX)/bin/i686-elf-ld $(GCC_SRC).tar.xz
	tar -xf gcc-$(GCC_VERSION).tar.xz
	mkdir $(GCC_BUILD)
	cd $(GCC_BUILD) && CFLAGS= ASMFLAGS= CC= CXX= LD= ASM= LINKFLAGS= LIBS= ../gcc-$(GCC_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)" 	\
		--target=$(TARGET)				\
		--disable-nls					\
		--enable-languages=c,c++		\
		--without-headers
	make -j$(CPU_CORES) -C $(GCC_BUILD) all-gcc all-target-libgcc
	make -C $(GCC_BUILD) install-gcc install-target-libgcc
	
$(GCC_SRC).tar.xz:
	wget $(GCC_URL)

clean-toolchain:
	rm -rf $(GCC_BUILD) $(GCC_SRC) $(BINUTILS_BUILD) $(BINUTILS_SRC)

clean-toolchain-all:
	rm -rf *
