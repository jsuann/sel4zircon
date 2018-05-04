#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"

/* Process syscalls */

void sys_process_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_exit(seL4_MessageInfo_t tag, uint64_t badge)
{
    /* No reply */
}

void sys_process_start(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_read_memory(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_write_memory(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

/* Thread syscalls */

void sys_thread_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t proc_handle = seL4_GetMR(0);
    uintptr_t user_name = seL4_GetMR(1);
    uint32_t name_len = seL4_GetMR(2);
    uint32_t options = seL4_GetMR(3);
    uintptr_t user_out = seL4_GetMR(4);

    if (options != 0) {
        return sys_reply(ZX_ERR_INVALID_ARGS);
    }

    // XXX

    sys_reply(ZX_OK);
}
