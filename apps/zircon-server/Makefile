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

# Targets
TARGETS := $(notdir $(SOURCE_DIR)).bin

# Declare that we are a rootserver for common.mk
APP_IS_ROOTSERVER := y

# Source files required to build the target
CFILES   := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))
CFILES   += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/utils/*.c))

CXXFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.cxx))

CXXFLAGS += -fno-rtti -fno-exceptions -fno-unwind-tables

# CPIO archive
OFILES := archive.o

HDRFILES := $(wildcard $(SOURCE_DIR)/include/*)

# Libraries required to build the target
LIBS = c sel4 sel4muslcsys \
       sel4simple sel4vka sel4allocman sel4vspace \
       sel4platsupport platsupport \
       sel4utils \
       cpio elf \
       utils sel4simple-default sel4debug zircon

include $(SEL4_COMMON)/common.mk

# targets to generate CPIO archive of elf files
${COMPONENTS}:
	false

archive.o: ${COMPONENTS}
	$(Q)mkdir -p $(dir $@)
	${COMMON_PATH}/files_to_obj.sh $@ _cpio_archive $^
