# [this project doesn't have an official name yet]
 A freestading 64-bit kernel (currently in a very early development stage)

At the beginning of July 2024, after watching some videos about OSDev, I decided to start developing my own 64-bit kernel, just for the fun and also to have a more complete idea of how an OS works under the hood.

## Why a kernel? Why not a whole OS?
An OS is mainly made out of two parts: the bootloader and the kernel.

The bootloader part is done by Limine, which offers a 64-bit environment out of the box (GRUB simply doesn't and you'll need to make your own "jump" from 32-bit to 64-bit), which is fine for our purpose of simply understanding how an OS works.

## The base
The core of this project is an adaptation of the code found in the [Limine Bare Bones](https://wiki.osdev.org/Limine_Bare_Bones) page of the OSDev wiki, which is a fundamental source when doing OS development.

## How to compile and run the OS
### REMINDER: Building of this project has been only tested on Linux, but if you can report anything about other platforms (Windows, MacOS, Ubuntu, ...) feel free to open an issue/PR! Here is a list of currently tested (and supported) platorms:

- Fedora Workstation 40 (Main machine)
- Debian 12 on WSL2 (Windows 10 21H2 build 19044, last tested on September 2024)

### Prequisites
There are some packages that are needed for building and running the OS, make sure to check your distro's repos for how to install them:

`nasm` - for compiling assembly files

`xorriso` - for creating the ISO

`qemu` - the emulator (you can use whatever software you like, but i have only tested with QEMU as of October 2024)

First, clone the repository:
`git clone https://github.com/RepubblicaTech/rtos.git`

To compile the OS, you'll not be able to use the system default C compiler (gcc), but you'll need a cross-compiler which will target a generic x86_64 platform rather than your specific one.

First, make sure to install the required dependencies for building gcc and binutils from source, and since they may vary for each distribution make sure to check out [this article](https://wiki.osdev.org/GCC_Cross-Compiler#Installing_Dependencies) on the OSDev wiki.

Thankfully, the [toolchain script made by nanobyte](https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/build_scripts/toolchain.mk) comes in handy regarding the whole downloading and compiling binutils and gcc from source, and I integrated it here to be simply run by a single command in the project directory:

(before running this command, enter the toolchain directory and change the CPU_CORES variable in the makefile according to the number of cores present in your CPU)

`make -C toolchain`

Since it needs to compile both binutils and gcc from source, it will take a long time (depending on your CPU cores and speed), so "you can go make yourself some coffee, and maybe watch the latest video from your favourite YouTube channel..."

\- [Nanobyte, 2021](https://youtu.be/TgIdFVOV_0U?t=709)

After this, you now have built your own toolchain for building this project but also any other one that relies on the base mentioned earlier (i'd suggest to update both `BINUTILS_VERSION` `GCC_VERSION` in the Makefile when an update of such package is available on your system and maybe re-run the `make -C toolchain` command, *if you always have time and will to do so*)

From now on, a simple `make` should compile the project's source code and create a bootable ISO image compatible with most computers with a 64-bit CPU.

(the first build might take a bit longer since it needs to download Limine and compile it, but it won't take as much as the toolchain one)

## Some useful make args

`make run` Compiles the code, creates a bootable ISO image and launches qemu with such image

`make debug` Same as above, but launches GDB according to the settings given by the `debug.gdb` file. Useful for kernel debugging with QEMU.

# Other useful stuff
I included a `.vscode` folder which includes the `c_cpp_properties.json` settings file that provides an enviroment specific to this project when using VSCode.

**Experimental**: for Zed users, I included a `settings.json` in the `.zed` folder (even tho it doesn't work for me, but if you have any fixes/tips about it, send an issue/PR :)
