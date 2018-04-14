#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

/* Prng just using rand(). Should be improved in future. */
/* Zircon gets entropy from HW (x86 only), CPU jitter and boot cmdline. */

unsigned int seed = 713281;

static inline void
init_prng(void)
{
    srand(seed);
}

static inline uint32_t
get_handle_rand(void)
{
    uint32_t secret = rand();
    /* Zircon clears top bit & bottom two bits */
    return (secret << 2) & INT_MAX;
}
