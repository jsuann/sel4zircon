#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "object/vmo.h"
#include "object/process.h"
#include "sys_helpers.h"

void sys_vmo_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

static void sys_vmo_read_write(seL4_MessageInfo_t tag, uint64_t badge, bool is_write)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t handle = seL4_GetMR(0);
    uintptr_t user_data = seL4_GetMR(1);
    uint64_t offset = seL4_GetMR(2);
    size_t len = seL4_GetMR(3);
    uintptr_t user_actual = seL4_GetMR(4);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    /* Get pointers */
    void *data;
    size_t *actual;
    /* XXX for this version of vmo_read, we read from 0 to len bytes */
    err = proc->uvaddr_to_kvaddr(user_data, 0, data);
    if (err) {
        return sys_reply(err);
    }
    err = proc->get_kvaddr(user_actual, actual);
    if (err) {
        return sys_reply(err);
    }

    /* Get VMO */
    ZxVmo *vmo;
    zx_rights_t required_right = (is_write) ? ZX_RIGHT_WRITE : ZX_RIGHT_READ;
    err = proc->get_object_with_rights(handle, required_right, vmo);
    if (err) {
        return sys_reply(err);
    }

    /* Check offset TODO also check cache state */
    if (offset >= vmo->get_size()) {
        return sys_reply(err);
    }

    /* Work out how much we can actually read */
    size_t actual_len = (len > vmo->get_size() - offset) ? (vmo->get_size() - offset) : len;

    /* Ensure required pages have been commited */
    if (!vmo->commit_range(offset, actual_len)) {
        return sys_reply(ZX_ERR_NO_MEMORY);
    }

    /* Do read/write */
    if (is_write) {
        vmo->write(offset, actual_len, data);
    } else {
        vmo->read(offset, actual_len, data);
    }
    *actual = actual_len;

    sys_reply(ZX_OK);
}

void sys_vmo_read(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_vmo_read_write(tag, badge, false);
}

void sys_vmo_write(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_vmo_read_write(tag, badge, false);
}
