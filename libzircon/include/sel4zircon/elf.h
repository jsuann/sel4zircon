#pragma once

#include <autoconf.h>
#include <zircon/types.h>

zx_status_t run_zircon_app(const char *filename, zx_handle_t *process,
        zx_handle_t *channel, uint64_t arg);
