#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"

void sys_handle_close(seL4_MessageInfo_t tag, uint64_t badge)
{
    zx_handle_t handle_value = seL4_GetMR(0);
    /* Closing a null handle is valid */
    if (handle_value == ZX_HANDLE_INVALID) {
        sys_reply(ZX_OK);
        return;
    }

    ZxProcess *proc = get_proc_from_badge(badge);

    Handle *h = proc->get_handle(handle_value);
    if (h == NULL) {
        sys_reply(ZX_ERR_BAD_HANDLE);
    } else {
        proc->remove_handle(h);
        ZxObject *o = h->get_object();
        if (o->destroy_handle(h)) {
            destroy_object(o);
        }
        sys_reply(ZX_OK);
    }
}

static void handle_dup_replace(bool is_replace, seL4_MessageInfo_t tag, uint64_t badge)
{
    //zx_handle_t handle_value = seL4_GetMR(0);
    //zx_rights_t rights = seL4_GetMR(1);
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
