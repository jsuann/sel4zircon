#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "syscalls.h"
#include "process.h"

void sys_process_exit(seL4_MessageInfo_t tag, uint32_t handle)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_create(seL4_MessageInfo_t tag, uint32_t handle)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_start(seL4_MessageInfo_t tag, uint32_t handle)
{
    if (seL4_MessageInfo_get_length(tag) != 3) {
        sys_reply(ZX_ERR_INVALID_ARGS);
        return;
    }
    printf("extra caps: %lu, caps unwrapped: %lx\n", seL4_MessageInfo_get_extraCaps(tag),
            seL4_MessageInfo_get_capsUnwrapped(tag));
    printf("%u\n", handle);
    printf("%lu %lu %lu\n", seL4_GetMR(0), seL4_GetMR(1), seL4_GetMR(2));
    //printf("%lx %lx\n", seL4_GetBadge(0), seL4_GetBadge(1));
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_read_memory(seL4_MessageInfo_t tag, uint32_t handle)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_write_memory(seL4_MessageInfo_t tag, uint32_t handle)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}
