#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "object/vmo.h"
#include "object/process.h"
#include "sys_helpers.h"

namespace SysVmo {
constexpr size_t kPageSize = 1 << seL4_PageBits;
/* Max size is kmap size minus guard page */
constexpr size_t kVmoMaxSize = ZX_VMO_SERVER_MAP_SIZE - kPageSize;
}

uint64_t sys_vmo_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 3);
    uint64_t size = seL4_GetMR(0);
    uint32_t options = seL4_GetMR(1);
    uintptr_t user_out = seL4_GetMR(2);

    if (options != 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* sel4zircon imposes size limit to ensure vmo fits in a kmap */
    if (size > SysVmo::kVmoMaxSize) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    zx_handle_t* out;
    err = proc->get_kvaddr(user_out, out);
    SYS_RET_IF_ERR(err);

    /* Calc the num pages */
    size_t num_pages = (size + SysVmo::kPageSize - 1) / SysVmo::kPageSize;

    /* Create the vmo */
    ZxVmo *vmo = allocate_object<ZxVmo>(num_pages);
    if (vmo == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Init the vmo */
    if (!vmo->init()) {
        free_object(vmo);
        return ZX_ERR_NO_MEMORY;
    }

    /* Create the handle */
    zx_handle_t vmo_handle = proc->create_handle_get_uval(vmo);
    if (vmo_handle == ZX_HANDLE_INVALID) {
        destroy_object(vmo);
        return ZX_ERR_NO_MEMORY;
    }

    *out = vmo_handle;
    return ZX_OK;
}

static uint64_t sys_vmo_read_write(seL4_MessageInfo_t tag, uint64_t badge, bool is_write)
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
    /* For this version of vmo read/write, we read from 0 to len bytes */
    err = proc->uvaddr_to_kvaddr(user_data, 0, data);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_actual, actual);
    SYS_RET_IF_ERR(err);

    /* Get VMO */
    ZxVmo *vmo;
    zx_rights_t required_right = (is_write) ? ZX_RIGHT_WRITE : ZX_RIGHT_READ;
    err = proc->get_object_with_rights(handle, required_right, vmo);
    SYS_RET_IF_ERR(err);

    /* Check offset */
    if (offset >= vmo->get_size()) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    /* Work out how much we can actually read */
    size_t actual_len = (len > vmo->get_size() - offset) ? (vmo->get_size() - offset) : len;

    /* Ensure required pages have been commited */
    if (!vmo->commit_range(offset, actual_len)) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Do read/write */
    if (is_write) {
        vmo->write(offset, actual_len, data);
    } else {
        vmo->read(offset, actual_len, data);
    }
    *actual = actual_len;

    return ZX_OK;
}

uint64_t sys_vmo_read(seL4_MessageInfo_t tag, uint64_t badge)
{
    return sys_vmo_read_write(tag, badge, false);
}

uint64_t sys_vmo_write(seL4_MessageInfo_t tag, uint64_t badge)
{
    return sys_vmo_read_write(tag, badge, true);
}

uint64_t sys_vmo_get_size(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 2);
    zx_handle_t handle = seL4_GetMR(0);
    uintptr_t user_size = seL4_GetMR(1);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    uint64_t* size;
    err = proc->get_kvaddr(user_size, size);
    SYS_RET_IF_ERR(err);

    ZxVmo *vmo;
    err = proc->get_object(handle, vmo);
    SYS_RET_IF_ERR(err);

    *size = vmo->get_size();
    return ZX_OK;
}

uint64_t sys_vmo_set_size(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 2);
    zx_handle_t handle = seL4_GetMR(0);
    uint64_t size = seL4_GetMR(1);

    if (size > SysVmo::kVmoMaxSize) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    ZxVmo *vmo;
    err = proc->get_object_with_rights(handle, ZX_RIGHT_WRITE, vmo);
    SYS_RET_IF_ERR(err);

    size_t num_pages = (size + SysVmo::kPageSize - 1) / SysVmo::kPageSize;

    /* Attempt resize */
    err = vmo->resize(num_pages);
    return err;
}

uint64_t sys_vmo_op_range(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 6);
    zx_handle_t handle = seL4_GetMR(0);
    uint32_t op = seL4_GetMR(1);
    uint64_t offset = seL4_GetMR(2);
    uint64_t size = seL4_GetMR(3);
    /* buffer & buffer_size (args 4 and 5) unused */

    /* Check for overflow */
    if (offset + size < offset) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    ZxVmo *vmo;
    /* Native zircon currently has no rights required, which is
       marked as an issue. Having write rights makes some sense. */
    err = proc->get_object_with_rights(handle, ZX_RIGHT_WRITE, vmo);
    SYS_RET_IF_ERR(err);

    /* Check offset & size */
    if ((offset + size) > vmo->get_size()) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    switch (op) {
    case ZX_VMO_OP_COMMIT:
        return (vmo->commit_range(offset, size)) ? ZX_OK : ZX_ERR_NO_MEMORY;
    case ZX_VMO_OP_DECOMMIT:
        vmo->decommit_range(offset, size);
        return ZX_OK;
    case ZX_VMO_OP_LOCK:
    case ZX_VMO_OP_UNLOCK:
    case ZX_VMO_OP_LOOKUP:
        return ZX_ERR_NOT_SUPPORTED;
    case ZX_VMO_OP_CACHE_SYNC:
    case ZX_VMO_OP_CACHE_INVALIDATE:
    case ZX_VMO_OP_CACHE_CLEAN:
    case ZX_VMO_OP_CACHE_CLEAN_INVALIDATE:
        /* Do nothing on x86 */
        return ZX_OK;
    default:
        return ZX_ERR_INVALID_ARGS;
    }
}
