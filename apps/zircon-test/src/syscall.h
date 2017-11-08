//#include <sel4/sel4.h>
#include "types.h"

#include "sys_def.h"
#include "sys_helper.h"

// TODO move to library

void
zx_null(zx_handle_t handle)
{
    ZX_SYSCALL_SEND(handle, ZX_SYS_NULL, 0);
    ZX_SYSCALL_SEND(handle, ZX_SYS_NULL, 1, a);
    ZX_SYSCALL_SEND(handle, ZX_SYS_NULL, 2, a, b);
    ZX_SYSCALL_SEND(handle, ZX_SYS_NULL, 3, a, b, c);
    ZX_SYSCALL_SEND(handle, ZX_SYS_NULL, 4, a, b, c, d);
    ZX_SYSCALL_SEND(handle, ZX_SYS_NULL, 5, a, b, c, d, e);
    ZX_SYSCALL_SEND(handle, ZX_SYS_NULL, 6, a, b, c, d, e, f);
    ZX_SYSCALL_SEND(handle, ZX_SYS_NULL, 7, a, b, c, d, e, f, g);
    ZX_SYSCALL_SEND(handle, ZX_SYS_NULL, 8, a, b, c, d, e, f, g, h);
}

zx_status_t
zx_handle_replace(zx_handle_t handle, zx_rights_t rights, zx_handle_t* out)
{
    ZX_SYSCALL_SEND(handle, ZX_SYS_HANDLE_REPLACE, 2, rights, (uintptr_t)out);
    *out = seL4_GetMR(1);
    return seL4_GetMR(0);
}
