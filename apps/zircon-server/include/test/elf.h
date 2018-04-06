#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "../object/process.h"

/* load elf segments into a test process */
uintptr_t load_elf_segments(ZxProcess *proc, const char *image_name);
