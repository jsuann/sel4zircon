#pragma once

#include <autoconf.h>

extern "C" {
#include <zircon/types.h>
#include <zircon/rights.h>
#include "env.h"
#include "debug.h"
}

void init_system_info(env_t *env);
uint64_t system_get_tsc_freq();
uint32_t system_get_num_cores();
