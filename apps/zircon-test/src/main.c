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
#include <sel4zircon/endpoint.h>

/* constants */
#define TEST_EP_ID      0xfee
#define TEST_EP_MSG     0xbce
#define TEST_EP_SLOT    ZX_THREAD_FIRST_FREE
#define TEST_EP_RIGHTS  ZX_ENDPOINT_ALL_RIGHTS

/* TODO replace with actual vmo */
uint8_t thrd_stack[8000];

zx_handle_t event_handle;
zx_handle_t ep_handle;

__attribute__((noreturn))
void thread_entry(uintptr_t arg1, uintptr_t arg2)
{
    printf("Thread entry! arg1: %lu, arg2 %lu\n", arg1, arg2);

    zx_nanosleep(zx_deadline_after(ZX_SEC(1)));
    assert(!zx_object_signal(event_handle, 0u, ZX_USER_SIGNAL_2));

    printf("Thread wait for msg\n");
    seL4_Word badge;
    seL4_Recv(TEST_EP_SLOT, &badge);
    printf("Thread got msg %lx, badge %lu\n", seL4_GetMR(0), badge);

    while (1) zx_nanosleep(zx_deadline_after(ZX_SEC(600)));
}

int main(int argc, char **argv) {
    seL4_MessageInfo_t tag;
    seL4_Word msg;

    /* Get starting handles
       TODO: replace with channel */
    tag = seL4_Recv(ZX_THREAD_FAULT_SLOT, &msg);
    zx_handle_t vmar_handle = seL4_GetMR(0);
    zx_handle_t proc_handle = seL4_GetMR(1);
    zx_handle_t thrd_handle = seL4_GetMR(2);
    zx_handle_t rsrc_handle = seL4_GetMR(3);

    printf(">=== Zircon Test ===\n");
    printf("Received handles: %u %u %u %u\n", vmar_handle, proc_handle,
            thrd_handle, rsrc_handle);

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

    /* try an invalid syscall num */
    tag = seL4_MessageInfo_new(10000, 0, 0, 0);
    seL4_Call(ZX_THREAD_SYSCALL_SLOT, tag);
    err = seL4_GetMR(0);
    assert(err == ZX_ERR_BAD_SYSCALL);

    assert(zx_handle_close(ZX_HANDLE_INVALID) == ZX_OK);
    assert(zx_handle_close(1231231313) == ZX_ERR_BAD_HANDLE);

    zx_handle_t thrd_handle2 = 0;
    err = zx_handle_replace(thrd_handle, ZX_RIGHT_SAME_RIGHTS, &thrd_handle2);
    printf("zx_handle_replace returned %d, new handle %u\n", err, thrd_handle2);
    /* Re-replace to restore thrd_handle */
    assert(!zx_handle_replace(thrd_handle2, ZX_RIGHT_SAME_RIGHTS, &thrd_handle));

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

    /* Create event */
    assert(!zx_event_create(0, &event_handle));

    /* Create a test thread */
    zx_handle_t new_thrd;
    const char *name = "thrd2";
    assert(!zx_thread_create(proc_handle, name, strlen(name), 0, &new_thrd));

    uintptr_t stack = ((uintptr_t)&thrd_stack[0]) + 8000;
    assert(!zx_thread_start(new_thrd, (uintptr_t)thread_entry, stack, 9, 6));

    /* Create test endpoint */
    assert(!zx_endpoint_create(rsrc_handle, TEST_EP_ID, 0, &ep_handle));
    printf("Created endpoint, handle %u\n", ep_handle);

    /* Mint a cap to each thread's cspace */
    assert(!zx_endpoint_mint_cap(ep_handle, thrd_handle, TEST_EP_SLOT, 1, TEST_EP_RIGHTS));
    assert(!zx_endpoint_mint_cap(ep_handle, new_thrd, TEST_EP_SLOT, 2, TEST_EP_RIGHTS));

    /* Do a wait to sync with other thread */
    zx_signals_t observed;
    assert(!zx_object_wait_one(event_handle, ZX_USER_SIGNAL_2,
            zx_deadline_after(ZX_SEC(10)), &observed));
    printf("Main thread woke from wait one.\n");

    /* Send a message to the other thread */
    printf("Sending msg to thread\n");
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, TEST_EP_MSG);
    seL4_Send(TEST_EP_SLOT, tag);

    zx_time_t deadline;
    deadline = zx_deadline_after(ZX_SEC(2));
    zx_nanosleep(deadline);
    printf("Main thread woke from sleep.\n");

    /* Delete endpoint caps from threads */
    assert(!zx_endpoint_delete_cap(ep_handle, thrd_handle, TEST_EP_SLOT));
    assert(!zx_endpoint_delete_cap(ep_handle, new_thrd, TEST_EP_SLOT));
    printf("Deleted endpoint caps.\n");

    uint64_t time1, time2;
    time1 = zx_clock_get(ZX_CLOCK_MONOTONIC);
    time2 = zx_clock_get(ZX_CLOCK_MONOTONIC);
    printf("Get time overhead: %lu\n", time2 - time1);
    uint64_t overhead = time2 - time1;

    time1 = zx_clock_get(ZX_CLOCK_MONOTONIC);
    for (size_t i = 0; i < 1000000; ++i) {
        zx_syscall_test_0();
        //zx_syscall_test_8(1,2,3,4,5,6,7,8);
    }
    time2 = zx_clock_get(ZX_CLOCK_MONOTONIC);
    printf("zx_syscall_test_0 time: %lu\n", ((time2 - time1) - overhead) / 1000000);

    /* Try to kill other thread */
    assert(!zx_task_kill(new_thrd));

    printf("Zircon test exiting!\n");

    zx_process_exit(0);

    printf("We shouldn't get here!\n");
    return 0;
}
