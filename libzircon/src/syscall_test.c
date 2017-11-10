
#include <autoconf.h>
#include <zircon/syscalls.h>

#include "sys_helper.h"

void
zx_null(zx_handle_t handle)
{
    ZX_SYSCALL_SEND(handle, ZX_SYS_NULL, 0);
}

// TODO move to syscall_handle.c
zx_status_t
zx_handle_replace(zx_handle_t handle, zx_rights_t rights, zx_handle_t* out)
{
    ZX_SYSCALL_SEND(handle, ZX_SYS_HANDLE_REPLACE, 2, rights, (uintptr_t)out);
    *out = seL4_GetMR(1);
    return seL4_GetMR(0);
}


zx_status_t
zx_syscall_test_0(void)
{
    ZX_SYSCALL_SEND(DEFAULT_HANDLE_CPTR, ZX_SYS_TEST_0, 0);
    return seL4_GetMR(0);
}

zx_status_t
zx_syscall_test_1(int a)
{
    ZX_SYSCALL_SEND(DEFAULT_HANDLE_CPTR, ZX_SYS_TEST_1, 1, a);
    return seL4_GetMR(0);
}

zx_status_t
zx_syscall_test_2(int a, int b)
{
    ZX_SYSCALL_SEND(DEFAULT_HANDLE_CPTR, ZX_SYS_TEST_2, 2, a, b);
    return seL4_GetMR(0);
}

zx_status_t
zx_syscall_test_3(int a, int b, int c)
{
    ZX_SYSCALL_SEND(DEFAULT_HANDLE_CPTR, ZX_SYS_TEST_3, 3, a, b, c);
    return seL4_GetMR(0);
}

zx_status_t
zx_syscall_test_4(int a, int b, int c, int d)
{
    ZX_SYSCALL_SEND(DEFAULT_HANDLE_CPTR, ZX_SYS_TEST_4, 4, a, b, c, d);
    return seL4_GetMR(0);
}

zx_status_t
zx_syscall_test_5(int a, int b, int c, int d, int e)
{
    ZX_SYSCALL_SEND(DEFAULT_HANDLE_CPTR, ZX_SYS_TEST_5, 5, a, b, c, d, e);
    return seL4_GetMR(0);
}

zx_status_t
zx_syscall_test_6(int a, int b, int c, int d, int e, int f)
{
    ZX_SYSCALL_SEND(DEFAULT_HANDLE_CPTR, ZX_SYS_TEST_6, 6, a, b, c, d, e, f);
    return seL4_GetMR(0);
}

zx_status_t
zx_syscall_test_7(int a, int b, int c, int d, int e, int f, int g)
{
    ZX_SYSCALL_SEND(DEFAULT_HANDLE_CPTR, ZX_SYS_TEST_7, 7, a, b, c, d, e, f, g);
    return seL4_GetMR(0);
}

zx_status_t
zx_syscall_test_8(int a, int b, int c, int d, int e, int f, int g, int h)
{
    ZX_SYSCALL_SEND(DEFAULT_HANDLE_CPTR, ZX_SYS_TEST_8, 8, a, b, c, d, e, f, g, h);
    return seL4_GetMR(0);
}
