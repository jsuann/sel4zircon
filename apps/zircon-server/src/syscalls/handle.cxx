#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "sys_helpers.h"
#include "debug.h"
}

extern "C" {
void sys_handle_close(seL4_MessageInfo_t tag, uint64_t badge);
void sys_handle_replace(seL4_MessageInfo_t tag, uint64_t badge);
void sys_handle_duplicate(seL4_MessageInfo_t tag, uint64_t badge);
}

void sys_handle_close(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_handle_replace(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_handle_duplicate(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}
