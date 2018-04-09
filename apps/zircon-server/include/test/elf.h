#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "../object/vmo.h"
#include "../object/process.h"
#include "../object/thread.h"

/* load elf segments into a test process */
uintptr_t load_elf_segments(ZxProcess *proc, const char *image_name,
        int &num_vmos, ZxVmo **&vmos);

/* start running elf program */
/* TODO */
