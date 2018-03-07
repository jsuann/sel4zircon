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
    //seL4_Word msg;

    printf(">=== Zircon Test ===\n");

    // test handle acquire syscall
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);
    tag = seL4_Call(EP_CPTR, tag);

    zx_handle_t handle = seL4_GetMR(0);
    printf(">received handle! %u\n", handle);

    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, 0xff);
    tag = seL4_Call(handle, tag);

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
    seL4_Call(handle, tag);
    err = seL4_GetMR(0);
    assert(err == ZX_ERR_BAD_SYSCALL);

    // XXX testing unwrapped caps
    err = zx_process_start(handle, handle, 0, 0, EP_CPTR, 0);
    printf("err = %d\n", err);

    test_cpp();

    return 0;
}
