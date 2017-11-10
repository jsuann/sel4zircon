#include "syscalls.h"

void sys_channel_create(seL4_MessageInfo_t tag, uint32_t handle)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_channel_read(seL4_MessageInfo_t tag, uint32_t handle)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_channel_write(seL4_MessageInfo_t tag, uint32_t handle)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_channel_call(seL4_MessageInfo_t tag, uint32_t handle)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}
