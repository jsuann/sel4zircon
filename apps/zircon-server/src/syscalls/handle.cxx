#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "object/handle.h"
#include "sys_helpers.h"

uint64_t sys_handle_close(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    zx_handle_t handle_value = seL4_GetMR(0);

    /* Closing a null handle is valid */
    if (handle_value == ZX_HANDLE_INVALID) {
        return ZX_OK;
    }

    ZxProcess *proc = get_proc_from_badge(badge);

    Handle *h = proc->get_handle(handle_value);

    if (h == NULL) {
        return ZX_ERR_BAD_HANDLE;
    }

    proc->remove_handle(h);
    destroy_handle_maybe_object(h);
    return ZX_OK;
}

static uint64_t handle_dup_replace(bool is_replace, seL4_MessageInfo_t tag,
        uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 3);
    zx_handle_t handle_val = seL4_GetMR(0);
    zx_rights_t rights = seL4_GetMR(1);
    uintptr_t user_out = seL4_GetMR(2);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    /* Get kvaddr of out */
    zx_handle_t *out;
    err = proc->get_kvaddr(user_out, out);
    SYS_RET_IF_ERR(err);

    Handle *src = proc->get_handle(handle_val);

    if (src == NULL) {
        return ZX_ERR_BAD_HANDLE;
    }

    if (!is_replace && !src->has_rights(ZX_RIGHT_DUPLICATE)) {
        return ZX_ERR_ACCESS_DENIED;
    }

    if (rights == ZX_RIGHT_SAME_RIGHTS) {
        rights = src->get_rights();
    } else if ((src->get_rights() & rights) != rights) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Create a duplicate */
    ZxObject *obj = src->get_object();
    Handle *dup = obj->create_handle(rights);

    if (dup == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    proc->add_handle(dup);
    *out = proc->get_handle_user_val(dup);

    if (is_replace) {
        proc->remove_handle(src);
        obj->destroy_handle(src);
        /* Since we just duped handle, we shouldn't be destroying the last! */
        assert(!obj->zero_handles());
    }

    return ZX_OK;
}

uint64_t sys_handle_replace(seL4_MessageInfo_t tag, uint64_t badge)
{
    return handle_dup_replace(true, tag, badge);
}

uint64_t sys_handle_duplicate(seL4_MessageInfo_t tag, uint64_t badge)
{
    return handle_dup_replace(false, tag, badge);
}
