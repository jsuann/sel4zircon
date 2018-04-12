/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <autoconf.h>
#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4utils/process.h>

#include <zircon/types.h>
#include <zircon/syscalls.h>

#include "test.h"

/* constants */
#define EP_CPTR SEL4UTILS_FIRST_FREE
#define MSG_DATA 0x2 //  arbitrary data to send

int main(int argc, char **argv) {
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    /* Get starting handles */
    tag = seL4_Recv(EP_CPTR, &msg);
    zx_handle_t vmar_handle = seL4_GetMR(0);
    zx_handle_t proc_handle = seL4_GetMR(1);
    zx_handle_t thrd_handle = seL4_GetMR(2);

    printf(">=== Zircon Test ===\n");
    printf("> Received handles: %u %u %u\n", vmar_handle, proc_handle, thrd_handle);

    zx_status_t err;
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;
    int e = 5;
    int f = 6;
    int g = 7;
    int h = 8;

    err = zx_syscall_test_0();
    printf("zx_syscall_test_0 returned %d\n", err);
    err = zx_syscall_test_1(a);
    printf("zx_syscall_test_1 returned %d\n", err);
    err = zx_syscall_test_2(a, b);
    printf("zx_syscall_test_2 returned %d\n", err);
    err = zx_syscall_test_3(a, b, c);
    printf("zx_syscall_test_3 returned %d\n", err);
    err = zx_syscall_test_4(a, b, c, d);
    printf("zx_syscall_test_4 returned %d\n", err);
    err = zx_syscall_test_5(a, b, c, d, e);
    printf("zx_syscall_test_5 returned %d\n", err);
    err = zx_syscall_test_6(a, b, c, d, e, f);
    printf("zx_syscall_test_6 returned %d\n", err);
    err = zx_syscall_test_7(a, b, c, d, e, f, g);
    printf("zx_syscall_test_7 returned %d\n", err);
    err = zx_syscall_test_8(a, b, c, d, e, f, g, h);
    printf("zx_syscall_test_8 returned %d\n", err);

    // try an invalid syscall no
    tag = seL4_MessageInfo_new(10000, 0, 0, 0);
    seL4_Call(EP_CPTR, tag);
    err = seL4_GetMR(0);
    assert(err == ZX_ERR_BAD_SYSCALL);

    err = zx_handle_close(ZX_HANDLE_INVALID);
    printf("zx_handle_close returned %d\n", err);

    err = zx_handle_close(1231231313);
    printf("zx_handle_close returned %d\n", err);

    zx_handle_t thrd_handle2 = 0;
    err = zx_handle_replace(thrd_handle, ZX_RIGHT_SAME_RIGHTS, &thrd_handle2);
    printf("zx_handle_replace returned %d, new handle %u\n", err, thrd_handle2);

    //err = zx_handle_close(vmar_handle);
    //printf("zx_handle_close returned %d\n", err);

    int stk = 0;
    void *ptr = malloc(4);
    printf("&stk: %p, ptr: %p\n", &stk, ptr);

    *((int *)0) = 0;

    printf("Zircon test exiting!\n");

    return 0;
}
