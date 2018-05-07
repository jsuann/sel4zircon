#include <autoconf.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"
#include "object/event.h"

uint64_t sys_event_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 2);
    uint32_t options = seL4_GetMR(0);
    uintptr_t user_out = seL4_GetMR(1);

    if (options != 0u) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    zx_handle_t *event_out;
    err = proc->get_kvaddr(user_out, event_out);
    SYS_RET_IF_ERR(err);

    ZxEvent *event;
    event = allocate_object<ZxEvent>();
    if (event == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    zx_handle_t event_handle = proc->create_handle_get_uval(event);
    if (event_handle == ZX_HANDLE_INVALID) {
        destroy_object(event);
        return ZX_ERR_NO_MEMORY;
    }

    *event_out = event_handle;
    return ZX_OK;
}
