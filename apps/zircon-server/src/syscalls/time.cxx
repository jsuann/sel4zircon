#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "object/thread.h"
#include "sys_helpers.h"

namespace SysTime {
int64_t utc_offset = 0;
}

void sys_deadline_after(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    uint64_t nanoseconds = seL4_GetMR(0);
    uint64_t deadline = get_system_time() + nanoseconds;
    sys_reply(deadline);
}

void sys_nanosleep(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    zx_time_t deadline = seL4_GetMR(0);
    ZxThread *thrd = get_thread_from_badge(badge);
    thrd->wait(nanosleep_cb, (void *)thrd, deadline, ZX_TIMER_SLACK_CENTER);
}

void sys_clock_get(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    uint32_t clock_id = seL4_GetMR(0);

    uint64_t time;
    switch (clock_id) {
    case ZX_CLOCK_MONOTONIC:
        time = get_system_time();
    case ZX_CLOCK_UTC:
        time = get_system_time() + SysTime::utc_offset;
    case ZX_CLOCK_THREAD:
        time = get_thread_from_badge(badge)->runtime_ns();
    default:
        time = 0;
    }

    sys_reply(time);
}
