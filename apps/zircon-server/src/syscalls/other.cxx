#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "object/process.h"
#include "sys_helpers.h"

namespace SysOther {
constexpr uint32_t kMaxDebugWriteSize = 256u;
}

uint64_t sys_undefined(seL4_MessageInfo_t tag, uint64_t badge)
{
    /* Get the syscall number */
    seL4_Word syscall = seL4_MessageInfo_get_label(tag);

    /* TODO Determine the calling process */
    dprintf(INFO, "Received unimplemented syscall: %lu\n", syscall);
    return ZX_ERR_NOT_SUPPORTED;
}

uint64_t sys_debug_write(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysOther;

    SYS_CHECK_NUM_ARGS(tag, 2);
    uintptr_t user_ptr = seL4_GetMR(0);
    uint32_t len = seL4_GetMR(1);

    if (len > kMaxDebugWriteSize) {
        len = kMaxDebugWriteSize;
    }

    zx_status_t err;
    void *buf;
    ZxProcess *proc = get_proc_from_badge(badge);

    err = proc->uvaddr_to_kvaddr(user_ptr, len, buf);
    SYS_RET_IF_ERR(err);

    dprintf(INFO, "(%s): ", proc->get_name());

    for (uint32_t i = 0; i < len; ++i) {
        putchar(((char *)buf)[i]);
    }
    putchar('\n');

    return len;
}

/* sel4zircon syscalls */
uint64_t sys_debug_putchar(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    char c = seL4_GetMR(0);
    putchar(c);
    return 0;
}
