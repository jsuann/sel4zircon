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

# Move start of elf sections to after USER_ASPACE_BASE
LDFLAGS += -Wl,--section-start=.text=1000120

CONFIG_HAVE_CRT :=

# Source files required to build the target
CFILES :=  $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))
ASMFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/crt0.S))
HDRFILES := $(wildcard $(SOURCE_DIR)/src/*.h)

# Libraries
LIBS := c sel4 zircon zxutils

include $(SEL4_COMMON)/common.mk
