#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
#include <zircon/types.h>
}

#include "object/timer.h"
#include "sys_helpers.h"

uint64_t sys_timer_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 3);
    uint32_t options = seL4_GetMR(0);
    uint32_t clock_id = seL4_GetMR(1);
    uintptr_t user_out = seL4_GetMR(2);

    if (clock_id != ZX_CLOCK_MONOTONIC || options > ZX_TIMER_SLACK_LATE) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    zx_handle_t *out;
    err = proc->get_kvaddr(user_out, out);
    SYS_RET_IF_ERR(err);

    ZxTimer *timer = allocate_object<ZxTimer>(options);

    if (timer == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    zx_handle_t timer_handle = proc->create_handle_get_uval(timer);

    if (timer_handle == ZX_HANDLE_INVALID) {
        destroy_object(timer);
        return ZX_ERR_NO_MEMORY;
    }

    *out = timer_handle;
    return ZX_OK;
}

uint64_t sys_timer_set(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 3);
    zx_handle_t handle = seL4_GetMR(0);
    zx_time_t deadline = seL4_GetMR(1);
    zx_duration_t slack = seL4_GetMR(2);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    ZxTimer *timer;
    err = proc->get_object_with_rights(handle, ZX_RIGHT_WRITE, timer);
    SYS_RET_IF_ERR(err);

    return timer->set(deadline, slack);
}

uint64_t sys_timer_cancel(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    zx_handle_t handle = seL4_GetMR(0);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    ZxTimer *timer;
    err = proc->get_object_with_rights(handle, ZX_RIGHT_WRITE, timer);
    SYS_RET_IF_ERR(err);

    return timer->cancel();
}
