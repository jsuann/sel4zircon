#pragma once

#include <autoconf.h>
#include <zircon/types.h>

extern void zx_debug_putchar(char c);

extern zx_status_t zx_get_elf_vmo(zx_handle_t hrsrc, zx_handle_t vmo_handle,
        const char *filename, uint32_t name_len, uint64_t *size);

extern uint64_t zx_get_ipc_buffer_addr(void);

extern void zx_start_server_bench(void);
extern void zx_end_server_bench(uint64_t *buf);
