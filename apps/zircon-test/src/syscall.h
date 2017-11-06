#include <sel4/sel4.h>
#include "types.h"

#define ZX_SYS_NULL             0

#define ZX_SYS_HANDLE_CLOSE     1
#define ZX_SYS_HANDLE_DUPLICATE 2
#define ZX_SYS_HANDLE_REPLACE   3

#define ZX_SYSCALL_SEND_2(syscall, a, b)  \
    

static inline void
zx_null(zx_handle_t handle)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(ZX_SYS_NULL, 0, 0, 0);
    seL4_Call(handle, tag);
}


static inline zx_status_t
zx_handle_replace(zx_handle_t handle, zx_rights_t rights, zx_handle_t* out)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(ZX_SYS_HANDLE_REPLACE, 0, 0, 2);
    seL4_SetMR(0, rights);
    seL4_SetMR(1, (uintptr_t)out);
    seL4_Call(handle, tag);
    *out = seL4_GetMR(1);
    return seL4_GetMR(0);
}
