#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"

namespace SysTasks {

constexpr uint64_t kStackAlign = 16;
constexpr uint64_t kStackAlignMask = ~(kStackAlign - 1);

}

/* Process syscalls */

void sys_process_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_exit(seL4_MessageInfo_t tag, uint64_t badge)
{
    /* No reply */
}

void sys_process_start(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_read_memory(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_write_memory(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

/* Thread syscalls */

void sys_thread_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t proc_handle = seL4_GetMR(0);
    uintptr_t user_buf = seL4_GetMR(1);
    uint32_t name_len = seL4_GetMR(2);
    uint32_t options = seL4_GetMR(3);
    uintptr_t user_out = seL4_GetMR(4);

    if (options != 0) {
        return sys_reply(ZX_ERR_INVALID_ARGS);
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    void *name_buf;
    zx_handle_t *out;
    err = proc->uvaddr_to_kvaddr(user_buf, name_len, name_buf);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_out, out);
    SYS_RET_IF_ERR(err);

    /* Get the process we are adding the thread to */
    ZxProcess *target_proc;
    err = proc->get_object(proc_handle, target_proc);
    SYS_RET_IF_ERR(err);

    ZxThread *thrd;
    thrd = allocate_object<ZxThread>();
    if (thrd == NULL) {
        return sys_reply(ZX_ERR_NO_MEMORY);
    }
    thrd->set_name((char *)name_buf);

    /* Init thread, add to target proc */
    if (!thrd->init() || !target_proc->add_thread(thrd)) {
        destroy_object(thrd);
        return sys_reply(ZX_ERR_NO_MEMORY);
    }

    /* Create handle */
    zx_handle_t thrd_handle = proc->create_handle_get_uval(thrd);
    if (thrd_handle == ZX_HANDLE_INVALID) {
        destroy_object(thrd);
        return sys_reply(ZX_ERR_NO_MEMORY);
    }

    *out = thrd_handle;
    sys_reply(ZX_OK);
}

void sys_thread_start(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t thrd_handle = seL4_GetMR(0);
    uintptr_t entry = seL4_GetMR(1);
    uintptr_t stack = seL4_GetMR(2);
    uintptr_t arg1 = seL4_GetMR(3);
    uintptr_t arg2 = seL4_GetMR(4);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    ZxThread *thrd;
    err = proc->get_object_with_rights(thrd_handle, ZX_RIGHT_WRITE, thrd);
    SYS_RET_IF_ERR(err);

    /* Check this isn't the first thread */
    if (!((ZxProcess *)thrd->get_owner())->is_running()) {
        return sys_reply(ZX_ERR_BAD_STATE);
    }

    /* Prepare stack: align, and pretend a return address is pushed */
    uintptr_t aligned_stack = stack & SysTasks::kStackAlignMask;
    aligned_stack -= sizeof(uintptr_t);

    /* Start thread */
    if (thrd->start_execution(entry, aligned_stack, arg1, arg2) != 0) {
        return sys_reply(ZX_ERR_BAD_STATE);
    }

    sys_reply(ZX_OK);
}
