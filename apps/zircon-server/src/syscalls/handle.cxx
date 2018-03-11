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
    zx_handle_t handle_value = seL4_GetMR(0);
    /* Closing a null handle is valid */
    if (handle_value == ZX_HANDLE_INVALID) {
        sys_reply(ZX_OK);
        return;
    }

    /* Get the calling process */

    /* Attempt to remove handle from process */

    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

static void handle_dup_replace(bool is_replace, seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_handle_replace(seL4_MessageInfo_t tag, uint64_t badge)
{
    handle_dup_replace(true, tag, badge);
}

void sys_handle_duplicate(seL4_MessageInfo_t tag, uint64_t badge)
{
    handle_dup_replace(false, tag, badge);
}
