#include <autoconf.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"

uint64_t sys_endpoint_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    return ZX_ERR_NOT_SUPPORTED;
}

uint64_t sys_endpoint_mint_cap(seL4_MessageInfo_t tag, uint64_t badge)
{
    return ZX_ERR_NOT_SUPPORTED;
}

uint64_t sys_endpoint_delete_cap(seL4_MessageInfo_t tag, uint64_t badge)
{
    return ZX_ERR_NOT_SUPPORTED;
}
