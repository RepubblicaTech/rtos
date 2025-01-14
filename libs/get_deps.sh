#!/bin/bash

set -e

# kernel source directory
KERNEL_DIR=$1
LIBS_DIR=$2

# required libraries
declare -a libs=("limine"
				"flanterm" 
				"nanoprintf")

stderr_echo() { 
	echo "$@" >&2; 
}

check_submodule() {
	# has the submodule been cloned
	if [ ! -d "$1" ]; then
		# clone it
		printf "Downloading and "
		git submodule update --init "$1" >> /dev/null
	fi
	# update it
	printf "Updating module $1\n"
	git submodule update --remote "$1"
	return 0
}

# $1: source file
# $2 target directory
copy_if_exists() {
	if [ ! -e "$1" ]; then
		stderr_echo "ERROR: file $1 does not exist. Quitting task..."
		return 1
	fi

	LAST_SLASH=${1##*/}
	if [[ $LAST_SLASH == *"."* ]]; then
		DIRECTORY=${2//$LAST_SLASH}
	else
		DIRECTORY=$2
	fi

	if [ ! -d "$DIRECTORY" ]; then
		stderr_echo "ERROR: target directory $2 not found. Creating it..."
		mkdir -p "$DIRECTORY"
		return 0
	fi

	echo "--> Copying: $1"
	echo "	  to: $2"
	cp -f "$1" "$2"
}

if [ -z "$KERNEL_DIR" ]; then
	stderr_echo "ERROR: You must give the ABSOLUTE path of your kernel source code directory (eg. libs)."
	exit 1
fi

forbidden_prefix="/"

if [ -z "$LIBS_DIR" ] || [[ "$LIBS_DIR" == "/"* ]]; then
	stderr_echo "ERROR: You must give the RELATIVE path of the external libraries (eg. src/kernel)."
	exit 1
fi

LIBS_FULL_DIR="$PWD/$LIBS_DIR"

# check if submodules have been downloaded
for lib in "${libs[@]}"
do
	check_submodule "$LIBS_FULL_DIR/$lib"
done

# copy required files

copy_if_exists $LIBS_DIR/limine/limine.h $KERNEL_DIR/system/limine.h
copy_if_exists $LIBS_DIR/nanoprintf/nanoprintf.h $KERNEL_DIR/system/nanoprintf.h

# copy flanterm headers
copy_if_exists $LIBS_DIR/flanterm/*.h $KERNEL_DIR/flanterm
copy_if_exists $LIBS_DIR/flanterm/*.c $KERNEL_DIR/flanterm
copy_if_exists $LIBS_DIR/flanterm/backends/*.h $KERNEL_DIR/flanterm/backends
copy_if_exists $LIBS_DIR/flanterm/backends/*.c $KERNEL_DIR/flanterm/backends

# custom font for flanterm
copy_if_exists $LIBS_DIR/patches/font.h $KERNEL_DIR/flanterm/backends

patch -u $KERNEL_DIR/flanterm/backends/fb.c -i $LIBS_DIR/patches/fb.c.patch