# [this project doesn't have an official name yet]
 A freestading 64-bit kernel (currently in a very early development stage)

At the beginning of July 2024, after watching some videos about OSDev, I decided to start developing my own 64-bit kernel, just for the fun and also to have a more complete idea of how an OS works under the hood.

## Why a kernel? Why not a whole OS?
An OS is mainly made out of two parts: the bootloader and the kernel.

The bootloader part is done by Limine, which offers a 64-bit environment out of the box (GRUB simply doesn't and you'll need to make your own "jump" from 32-bit to 64-bit), which is fine for our purpose of simply understanding how an OS works.

## Current development state
Go check [The roadmap](docs/Roadmap.md).

## The base
The core of this project is an adaptation of the code found in the [Limine Bare Bones](https://wiki.osdev.org/Limine_Bare_Bones) page of the OSDev wiki, which is a fundamental source when doing OS development.

## How to compile and run the OS

Contents:

1. [Prerequisistes](#prequisites)
2. [Setting up a cross-compiler](#setting-up-a-cross-compiler)
	- [Prebuilt option](#prebuilt-option)
	- ["Do It Yourself" Version](#do-it-yourself-version)
3. [Actually compiling the project](#actually-compiling-the-project)

Extras:
- [Some useful make args](#some-useful-make-args)
- [Other useful stuff](#other-useful-stuff)

### REMINDER: Building of this project has been only tested on Linux, but if you can report anything about other platforms (Windows, MacOS, Ubuntu, ...) feel free to open an issue/PR! Here is a list of currently tested (and supported) platorms:

- Fedora Workstation 41 (Main machine, at least one build/week)
- Debian 12 on WSL2 (Windows 10 21H2 build 19044, last tested on September 2024)
- Arch Linux (last tested on December 2024)

### Prequisites
There are some packages that are needed for building and running the OS, make sure to check your distro's repos for how to install them:

`nasm` - for compiling assembly files

`xorriso` - for creating the ISO

`qemu` - the emulator (you can use whatever software you like, but i have only tested with QEMU as of October 2024)

`diff` and `patch` - they are required to edit Flanterm's source files to point to the "correct" header files (due to the current project structure)

Clone the repository:

`git clone https://github.com/RepubblicaTech/rtos.git`

Then, you've got to download all the required submodules (nanoprintf, Flanterm and Limine):

`git submodule update --init --recursive`

Running `make deps` will copy all the required modules to the project directory, and patch those files if needed.
(make sure to run this every once in a while to make sure all dependencies will be updated in the kernel too)

### Setting up a cross-compiler

To compile the OS, you'll not be able to use the system default C compiler (gcc), but you'll need a cross-compiler which will target a generic x86_64 platform rather than your specific one.

#### Prebuilt option

If you either are lazy or don't have time for compiling the toolchain, you can grab a pre-made one from [here](https://newos.org/toolchains/) (you should grab the one that says `x86_64-elf-[the highest version number you can find]-[Linux/FreeBSD/Darwin]-x86_64.tar.xz`, and extract the contents of the folder inside the xz to a `x86_64-elf` directory inside the `toolchain` folder).
You can now proceed to [actually compiling the project](#actually-compiling-the-project).

You should end up with a structure like this:
```
rtos
│...
├── src
│   └── ...
└── toolchain
    ├── Makefile
    └── x86_64-elf	<-- You should create this directory
	--- All of these files should come from the downloaded tar archive ---
        ├── bin
        ├── include
        ├── lib
        ├── libexec
        ├── share
        └── x86_64-elf

```

#### "Do It Yourself" version

First, make sure to install the required dependencies for building gcc and binutils from source, and since they may vary for each distribution make sure to check out [this article](https://wiki.osdev.org/GCC_Cross-Compiler#Installing_Dependencies) on the OSDev wiki.

Thankfully, the [toolchain script made by nanobyte](https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/build_scripts/toolchain.mk) comes in handy regarding the whole downloading and compiling binutils and gcc from source, and I integrated it here to be simply run by a single command in the project directory:

*(arguments in square brackets are optional)*

`make -C toolchain [CPU_CORES=<number of desired cores, defaults to $(nproc)>]`

(**NOTE**: if you actually manage to port this kernel to another architecture supported by Limine, you can add a `TARGET_BASE` or `TARGET` argument to compile a different toolchain. Same applies to the [project Makefile](Makefile).)

Since it needs to compile both binutils and gcc from source, it will take a long time (depending on your CPU cores and speed), so "you can go make yourself some coffee, and maybe watch the latest video from your favourite YouTube channel..."

\- [Nanobyte, 2021](https://youtu.be/TgIdFVOV_0U?t=709)

After this, you now have built your own toolchain for building this project but also any other one that relies on the base mentioned earlier (i'd suggest to update both `BINUTILS_VERSION` `GCC_VERSION` in the Makefile when an update of such package is available on your system and maybe re-run the `make -C toolchain` command, *if you always have time and will to do so*).

### Actually compiling the project

As of November 2024, project gets compiled with binutils version 2.43 and gcc 14.2.0, which are the versions mentioned in the Makefile.

From now on, a simple `make` should compile the project's source code and create a bootable ISO image compatible with most computers with a 64-bit CPU.

(the first build might take a bit longer since it needs to download Limine and compile it, but it won't take as much as the toolchain one)

## Some useful make args

`make run` Compiles the code, creates a bootable ISO image and launches qemu with such image

`make debug` Same as above, but launches GDB according to the settings given by the `debug.gdb` file. Useful for kernel debugging with QEMU.

# Other useful stuff
I included a `.vscode` folder which includes the `c_cpp_properties.json` settings file that provides an enviroment specific to this project when using VSCode.

## **Update! (18/12/2024)** 

You can finally code in Zed without getting annoyed by all the errors a normal clang compiler would emit.
Setting it up is pretty simple:

1. Install the `bear` package (check your distro's package manager/install it from [Brew](https://formulae.brew.sh/formula/bear))
2. In a terminal inside the project directory, run `bear -- make`. This will both compile and generate a `compile_commands.json` file that `clangd` (and Zed) will use.
3. Open Zed, and if you encounter only warnings in the code, then you're done. Enjoy editing!

P.S. if Zed sometimes won't respond to certain error fixes, try to restart the language server via the command palette, and then all errors should be gone.
