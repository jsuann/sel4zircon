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

# Make sure this symbol stays around as we don't reference this, but
# whoever loads us will
LDFLAGS += -u __vsyscall_ptr

# Move start of elf sections to after USER_ASPACE_BASE
LDFLAGS += -Wl,--section-start=.init=1000120

# Source files required to build the target
CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))
CXXFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.cxx))

HDRFILES := $(wildcard $(SOURCE_DIR)/src/*.h)

# Libraries
LIBS := c sel4 sel4muslcsys elf utils zircon zxutils

include $(SEL4_COMMON)/common.mk
