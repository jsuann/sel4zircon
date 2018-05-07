#include <autoconf.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"
#include "object/waiter.h"

namespace SysObjectWait {
constexpr uint32_t kMaxWaitHandleCount = 8u;
}

uint64_t sys_object_wait_one(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 4);
    zx_handle_t handle_value = seL4_GetMR(0);
    zx_signals_t signals = seL4_GetMR(1);
    zx_time_t deadline = seL4_GetMR(2);
    uintptr_t user_observed = seL4_GetMR(3);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    zx_signals_t *observed;
    /* Observed can either be NULL or a valid pointer */
    if (user_observed != 0) {
        err = proc->get_kvaddr(user_observed, observed);
        SYS_RET_IF_ERR(err);
    } else {
        observed = NULL;
    }

    Handle *h = proc->get_handle(handle_value);
    if (h == NULL) {
        return ZX_ERR_BAD_HANDLE;
    }
    if (!h->has_rights(ZX_RIGHT_WAIT)) {
        return ZX_ERR_ACCESS_DENIED;
    }

    /* Check if required signals already met */
    zx_signals_t initial_state = h->get_object()->get_signals();
    if (initial_state & signals) {
        if (observed != NULL) {
            *observed = initial_state;
        }
        return ZX_OK;
    }

    /* Get thread & attempt wait */
    ZxThread *thrd = get_thread_from_badge(badge);
    err = thrd->obj_wait_one(h, signals, deadline, observed);
    SYS_RET_IF_ERR(err);

    /* Wait was successful, so don't reply */
    server_should_not_reply();
    return 0;
}
