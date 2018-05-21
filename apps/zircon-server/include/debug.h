#pragma once

#include <autoconf.h>

#include <stdio.h>

/* debug levels */
#define CRITICAL    0
#define ALWAYS      0
#define INFO        1
#define SPEW        2

#define ZX_DEBUGLEVEL   0

#define ZX_PRINT_FLUSH  0

#if ZX_PRINT_FLUSH
#define zx_printf(x...) printf("[ZX] " x); fflush(stdout);
#else
#define zx_printf(x...) printf("[ZX] " x);
#endif

#define dprintf(level, x...) do { if ((level) <= ZX_DEBUGLEVEL) { zx_printf(x); } } while (0)
