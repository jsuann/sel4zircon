#include <autoconf.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"
#include "object/object.h"

uint64_t sys_object_signal(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 3);
    zx_handle_t handle_value = seL4_GetMR(0);
    uint32_t clear_mask = seL4_GetMR(1);
    uint32_t set_mask = seL4_GetMR(2);

    ZxProcess *proc = get_proc_from_badge(badge);

    Handle *h = proc->get_handle(handle_value);
    if (h == NULL) {
        return ZX_ERR_BAD_HANDLE;
    }

    if (!h->has_rights(ZX_RIGHT_SIGNAL)) {
        return ZX_ERR_ACCESS_DENIED;
    }

    return h->get_object()->user_signal(clear_mask, set_mask, false);
}

uint64_t sys_object_signal_peer(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 3);
    zx_handle_t handle_value = seL4_GetMR(0);
    uint32_t clear_mask = seL4_GetMR(1);
    uint32_t set_mask = seL4_GetMR(2);

    ZxProcess *proc = get_proc_from_badge(badge);

    Handle *h = proc->get_handle(handle_value);
    if (h == NULL) {
        return ZX_ERR_BAD_HANDLE;
    }

    if (!h->has_rights(ZX_RIGHT_SIGNAL_PEER)) {
        return ZX_ERR_ACCESS_DENIED;
    }

    return h->get_object()->user_signal(clear_mask, set_mask, true);
}
