#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

lib-dirs:=libs

# The main target we want to generate
all: app-images

-include .config

include tools/common/project.mk

# tests to run
ifeq (${TEST}y,y)
	TEST=".*"
endif

# objdump args
ifeq (${OBJFLAGS}y,y)
    OBJFLAGS="Dlx"
endif

# test that all configs compile
compile-all:
	${PWD}/projects/sel4test/tools/compile-all.sh

# objdump the kernel image
objdump-kernel:
	${CONFIG_CROSS_COMPILER_PREFIX}objdump -${OBJFLAGS} stage/${ARCH}/${PLAT}/kernel.elf

# objdump driver app
objdump-driver:
	${CONFIG_CROSS_COMPILER_PREFIX}objdump -${OBJFLAGS} stage/${ARCH}/${PLAT}/bin/sel4test-driver

# objdump tests app
objdump-tests:
	${CONFIG_CROSS_COMPILER_PREFIX}objdump -${OBJFLAGS} stage/${ARCH}/${PLAT}/bin/sel4test-tests

# pick a test or subset of tests to run
# usage: make select-test TEST=<regexp>
select-test:
	sed -i "s/CONFIG_TESTPRINTER_REGEX=\".*\"/CONFIG_TESTPRINTER_REGEX=\"${TEST}\"/" .config
	@echo "Selected test ${TEST}"

simulate-x86_64:
	qemu-system-x86_64 \
        -m 2048 -nographic -kernel images/kernel-x86_64-pc99 \
        -initrd images/zircon-server-image-x86_64-pc99 -cpu Haswell

# Some example image builds (NOTE: may need to adapt addresses)
build-binary: images/${apps}-image-${ARCH}-${PLAT}
	${CONFIG_CROSS_COMPILER_PREFIX}objcopy -O binary \
	images/${apps}-image-${ARCH}-${PLAT} images/sel4test.bin
	@echo "1. put file images/sel4test.bin into SD card root directory"
	@echo "2. At U-Boot prompt, enter:"
	@echo "    > mmc dev 1"
	@echo "    > ext2load mmc ${disk}:1 20000000 sel4test.bin"
	@echo "    > go 20000000"

build-uImage: images/${apps}-image-${ARCH}-${PLAT}
	mkimage -A arm -a 0x30000000 -e 0x30000000 -C none \
	-A ${ARCH} -T kernel -O qnx \
	-d images/${apps}-image-${ARCH}-${PLAT} images/sel4test.uImage
	@echo "1. put file images/sel4test.uImage into SD card root directory"
	@echo "2. At U-Boot prompt, enter:"
	@echo "    > mmc dev 1"
	@echo "    > ext2load mmc ${disk}:1 10800000 sel4test.uImage"
	@echo "    > bootm 10800000"

.PHONY: help
help:
	@echo "sel4test - unit and regression tests for seL4"
	@echo " make menuconfig      - Select build configuration via menus."
	@echo " make <defconfig>     - Apply one of the default configurations. See"
	@echo "                        below for valid configurations."
	@echo " make silentoldconfig - Update configuration with the defaults of any"
	@echo "                        newly introduced settings."
	@echo " make                 - Build with the current configuration."
	@echo ""
	@echo "Valid default configurations are:"
	@ls -1 configs | sed -e 's/\(.*\)/\t\1/g'
