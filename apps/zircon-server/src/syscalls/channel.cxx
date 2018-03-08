#include <autoconf.h>

extern "C" {
#include "sys_helpers.h"
#include "debug.h"
}

extern "C" {
void sys_channel_create(seL4_MessageInfo_t tag, uint64_t badge);
void sys_channel_read(seL4_MessageInfo_t tag, uint64_t badge);
void sys_channel_write(seL4_MessageInfo_t tag, uint64_t badge);
void sys_channel_call(seL4_MessageInfo_t tag, uint64_t badge);
}


void sys_channel_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_channel_read(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_channel_write(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_channel_call(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}
