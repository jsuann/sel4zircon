#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "syscalls.h"
#include "handle.h"

void sys_handle_close(seL4_MessageInfo_t tag, uint32_t handle)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_handle_replace(seL4_MessageInfo_t tag, uint32_t handle)
{
    printf("sys handle replace: %u\n", handle);

    // example error check
    if (seL4_MessageInfo_get_length(tag) != 2) {
        sys_reply(ZX_ERR_INVALID_ARGS);
        return;
    }

    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_handle_duplicate(seL4_MessageInfo_t tag, uint32_t handle)
{
    printf("sys handle duplicate: %u\n", handle);
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}
