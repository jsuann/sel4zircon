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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <zircon/types.h>
#include <zircon/syscalls.h>
#include <sel4zircon/cspace.h>

/* constants */
#define EP_CPTR ZX_THREAD_SYSCALL_SLOT
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
    printf("Received handles: %u %u %u\n", vmar_handle, proc_handle, thrd_handle);

    char *hello_msg = "Hello zircon server!";
    zx_debug_write((void *)hello_msg, strlen(hello_msg));

    zx_status_t err;

    assert(zx_syscall_test_0() == 0);
    assert(zx_syscall_test_1(1) == 1);
    assert(zx_syscall_test_2(1,2) == 3);
    assert(zx_syscall_test_3(1,2,3) == 6);
    assert(zx_syscall_test_4(1,2,3,4) == 10);
    assert(zx_syscall_test_5(1,2,3,4,5) == 15);
    assert(zx_syscall_test_6(1,2,3,4,5,6) == 21);
    assert(zx_syscall_test_7(1,2,3,4,5,6,7) == 28);
    assert(zx_syscall_test_8(1,2,3,4,5,6,7,8) == 36);
    assert(zx_syscall_test_wrapper(20, 20, 60) == 100);

    // try an invalid syscall no
    tag = seL4_MessageInfo_new(10000, 0, 0, 0);
    seL4_Call(EP_CPTR, tag);
    err = seL4_GetMR(0);
    assert(err == ZX_ERR_BAD_SYSCALL);

    assert(zx_handle_close(ZX_HANDLE_INVALID) == ZX_OK);
    assert(zx_handle_close(1231231313) == ZX_ERR_BAD_HANDLE);

    zx_handle_t thrd_handle2 = 0;
    err = zx_handle_replace(thrd_handle, ZX_RIGHT_SAME_RIGHTS, &thrd_handle2);
    printf("zx_handle_replace returned %d, new handle %u\n", err, thrd_handle2);

    int stk = 0;
    void *ptr = malloc(4);
    printf("&stk: %p, ptr: %p\n", &stk, ptr);

    printf("FIFO TEST\n");
    zx_handle_t fifo1, fifo2;
    err = zx_fifo_create(16, sizeof(uint64_t), 0, &fifo1, &fifo2);
    printf("fifo create: ret %d, fifo1 %u, fifo2 %u\n", err, fifo1, fifo2);

    uint64_t arr1[8] = {1,2,3,4,5,6,7,8};
    uint64_t arr2[8] = {0};
    uint32_t num;
    size_t size = 8 * sizeof(uint64_t);
    err = zx_fifo_write(fifo1, &arr1, size, &num);
    printf("fifo write: ret %d\n", err);

    err = zx_fifo_read(fifo2, &arr2, size, &num);
    printf("fifo read: ret %d\n", err);
    for (int i = 0; i < 8; ++i) {
        assert(arr1[i] == arr2[i]);
    }

    assert(!zx_handle_close(fifo1));
    assert(!zx_handle_close(fifo2));
    //assert(!zx_fifo_create(16, sizeof(uint64_t), 0, &fifo1, &fifo2));

    printf("Zircon test exiting!\n");

    /* TODO move this to exit? */
    zx_process_exit(0);

    printf("We shouldn't get here!\n");
    return 0;
}
