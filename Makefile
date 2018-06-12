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

simulate-x86_64:
	qemu-system-x86_64 \
        -m 4096 -nographic -kernel images/kernel-x86_64-pc99 \
        -initrd images/zircon-server-image-x86_64-pc99 -cpu Haswell

run-zircon:
	qemu-system-x86_64 \
        -m 2048 -nographic -kernel projects/zircon-src/build-x86-release/zircon.bin \
        -initrd projects/zircon-src/build-x86-release/bootdata.bin -cpu Haswell -append userboot=bin/sel4zircon-test

mqrun:
	mq.sh run -s skylake -c "Zircon test exiting!" -f images/kernel-x86_64-pc99 -f images/zircon-server-image-x86_64-pc99

mq-zircon:
	mq.sh run -s skylake -c "Zircon test exiting!" -f projects/zircon-src/build-x86-release/zircon.bin \
        -f projects/zircon-src/build-x86-release/bootdata.bin
