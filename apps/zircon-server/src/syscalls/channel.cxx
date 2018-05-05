#include <autoconf.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"

uint64_t sys_channel_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    return ZX_ERR_NOT_SUPPORTED;
}

uint64_t sys_channel_read(seL4_MessageInfo_t tag, uint64_t badge)
{
    return ZX_ERR_NOT_SUPPORTED;
}

uint64_t sys_channel_write(seL4_MessageInfo_t tag, uint64_t badge)
{
    return ZX_ERR_NOT_SUPPORTED;
}

uint64_t sys_channel_call(seL4_MessageInfo_t tag, uint64_t badge)
{
    return ZX_ERR_NOT_SUPPORTED;
}
