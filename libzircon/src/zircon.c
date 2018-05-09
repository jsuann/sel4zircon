#include <sel4/sel4.h>

#include <zircon/syscalls.h>
#include <sel4zircon/cspace.h>
#include <sel4zircon/debug.h>
#include <sel4zircon/endpoint.h>

#include "sys_def.h"

/* Endpoint for performing zircon syscalls */
#define ZX_SYSCALL_CPTR ZX_THREAD_SYSCALL_SLOT

/*
 * Macros for performing syscalls
 */

#define ZX_SYSCALL_SEND(syscall, n, ...) \
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(syscall, 0, 0, n); \
    ZX_SET_ARGS(n, __VA_ARGS__) \
    seL4_Call(ZX_SYSCALL_CPTR, tag)

#define ZX_SET_ARGS(n, ...) ZX_SET_ARGS_##n(0, __VA_ARGS__)

#define ZX_SET_ARGS_0(...)
#define ZX_SET_ARGS_1(x, arg)       seL4_SetMR(x, (seL4_Word)arg);
#define ZX_SET_ARGS_2(x, arg, ...)  seL4_SetMR(x, (seL4_Word)arg); ZX_SET_ARGS_1(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_3(x, arg, ...)  seL4_SetMR(x, (seL4_Word)arg); ZX_SET_ARGS_2(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_4(x, arg, ...)  seL4_SetMR(x, (seL4_Word)arg); ZX_SET_ARGS_3(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_5(x, arg, ...)  seL4_SetMR(x, (seL4_Word)arg); ZX_SET_ARGS_4(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_6(x, arg, ...)  seL4_SetMR(x, (seL4_Word)arg); ZX_SET_ARGS_5(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_7(x, arg, ...)  seL4_SetMR(x, (seL4_Word)arg); ZX_SET_ARGS_6(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_8(x, arg, ...)  seL4_SetMR(x, (seL4_Word)arg); ZX_SET_ARGS_7(INC(x), __VA_ARGS__)

#define INC(x)  INC_##x
#define INC_0   1
#define INC_1   2
#define INC_2   3
#define INC_3   4
#define INC_4   5
#define INC_5   6
#define INC_6   7
#define INC_7   8

/* Actual syscall defs are autogenerated */
#include "zx_calls.def"

/* sel4zircon specific calls defined below */
void zx_debug_putchar(char c)
{
    ZX_SYSCALL_SEND(ZX_SYS_DEBUG_PUTCHAR, 1, c);
}

zx_status_t zx_endpoint_create(zx_handle_t resource,
        uint64_t id, uint32_t options, zx_handle_t *out)
{
    ZX_SYSCALL_SEND(ZX_SYS_ENDPOINT_CREATE, 4, resource, id, options, out);
    return seL4_GetMR(0);
}

zx_status_t zx_endpoint_mint_cap(zx_handle_t endpoint,
        zx_handle_t thread, seL4_CPtr slot, seL4_Word badge, seL4_Word rights)
{
    ZX_SYSCALL_SEND(ZX_SYS_ENDPOINT_MINT_CAP, 5, endpoint, thread, slot, badge, rights);
    return seL4_GetMR(0);
}

zx_status_t zx_endpoint_delete_cap(zx_handle_t endpoint,
        zx_handle_t thread, seL4_CPtr slot)
{
    ZX_SYSCALL_SEND(ZX_SYS_ENDPOINT_DELETE_CAP, 3, endpoint, thread, slot);
    return seL4_GetMR(0);
}
