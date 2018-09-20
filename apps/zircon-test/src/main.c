#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <zircon/types.h>
#include <zircon/syscalls.h>
#include <zircon/stack.h>
#include <zircon/status.h>
#include <zircon/process.h>

#ifdef CONFIG_HAVE_SEL4ZIRCON
#include <sel4zircon/cspace.h>
#include <sel4zircon/endpoint.h>
#include <sel4zircon/debug.h>
#include <sel4zircon/elf.h>
#endif

#include <mini-process/mini-process.h>

#include "bench.h"

#define DO_BENCHMARK    0

/* constants */
#define TEST_EP_ID      0xfee
#define TEST_EP_MSG     0xbce
#define TEST_EP_SLOT    ZX_THREAD_FIRST_FREE
#define TEST_EP_RIGHTS  ZX_ENDPOINT_ALL_RIGHTS

#define NUM_CHANNEL_RUNS    10
#define CHANNEL_BUF_SIZE    10000

zx_handle_t event_handle;
zx_handle_t ep_handle;
zx_handle_t socket0, socket1;
zx_handle_t ch0, ch1;

void run_hello_world(void)
{
    zx_handle_t process, channel;
    assert(!run_zircon_app("hello-world", &process, &channel, 0));

    char writebuf[CHANNEL_BUF_SIZE];
    char readbuf[CHANNEL_BUF_SIZE];

    printf("Starting ping-pong test with hello-world app...\n");
    for (int i = 0; i < NUM_CHANNEL_RUNS; ++i) {
        printf("Sending message %d to hello-world...\n", i);
        /* Write some random data to the buffer */
        for (int j = 0; j < CHANNEL_BUF_SIZE; j += sizeof(int)) {
            int *val = (int *)&writebuf[j];
            *val = rand();
        }
        /* Send to hello world app */
        assert(!zx_channel_write(channel, 0, writebuf, CHANNEL_BUF_SIZE, NULL, 0));
        /* Wait for response */
        assert(!zx_object_wait_one(channel, ZX_CHANNEL_READABLE,  ZX_TIME_INFINITE, NULL));
        /* Get message and check contents match */
        assert(!zx_channel_read(channel, 0, readbuf, NULL, CHANNEL_BUF_SIZE, 0, NULL, NULL));
        assert(!memcmp(writebuf, readbuf, CHANNEL_BUF_SIZE));
        //zx_nanosleep(zx_deadline_after(ZX_MSEC(200)));
    }

    zx_task_kill(process);
}

/* Test thread function. Zircon threads map to seL4 threads. */
__attribute__((noreturn))
void thread_entry(uintptr_t arg1, uintptr_t arg2)
{
    printf("Test thread entry! arg1: %lu, arg2 %lu\n", arg1, arg2);

    char buf[CHANNEL_BUF_SIZE] = {0};
    zx_status_t err = zx_socket_read(socket1, 0, buf, 50, NULL);
    assert(!err);
    printf("Test thread: socket message from main thread: %s\n", buf);

    /* Send signal to main thread */
    zx_nanosleep(zx_deadline_after(ZX_SEC(1)));
    assert(!zx_object_signal(event_handle, 0u, ZX_USER_SIGNAL_2));

    /* Endpoint test */
    printf("Test thread: waiting for message...\n");
    seL4_Word badge;
    seL4_Recv(TEST_EP_SLOT, &badge);
    printf("Test thread got msg %lx, badge %lu\n", seL4_GetMR(0), badge);

    /* Wait for signal from main thread */
    assert(!zx_object_wait_one(event_handle, ZX_USER_SIGNAL_3, ZX_TIME_INFINITE, NULL));

    /* Ping pong test with main thread */
    for (int i = 0; i < NUM_CHANNEL_RUNS; ++i) {
        assert(!zx_object_wait_one(ch1, ZX_CHANNEL_READABLE,  ZX_TIME_INFINITE, NULL));
        printf("Test thread received channel message %d\n", i);
        assert(!zx_channel_read(ch1, 0, buf, NULL, CHANNEL_BUF_SIZE, 0, NULL, NULL));
        assert(!zx_channel_write(ch1, 0, buf, CHANNEL_BUF_SIZE, NULL, 0));
    }

    while (1) zx_nanosleep(zx_deadline_after(ZX_SEC(600)));
}

int main(int argc, char **argv) {
    seL4_MessageInfo_t tag;

    printf("=== Zircon Test ===\n");

#ifdef CONFIG_HAVE_SEL4ZIRCON
    char *hello_msg = "Hello zircon server!";
    zx_debug_write((void *)hello_msg, strlen(hello_msg));

    zx_init_startup_handles(argv);
#endif

#if DO_BENCHMARK
    run_benchmarks();
#endif

    /* Get init handles */
    zx_handle_t thrd_handle = zx_thread_self();
    zx_handle_t proc_handle = zx_process_self();
    zx_handle_t vmar_handle = zx_vmar_root_self();
    zx_handle_t rsrc_handle = zx_resource_root();
    zx_handle_t job_handle = zx_job_default();

    printf("Received init handles: %u %u %u %u %u\n", vmar_handle, proc_handle,
            thrd_handle, rsrc_handle, job_handle);

    /* Check test syscalls */
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

    /*
     * Try an invalid syscall number. This is how Zircon syscalls
     * are performed on seL4 - each thread has an endpoint cap used for
     * making the syscalls. See libzircon for the valid syscalls.
     */
    tag = seL4_MessageInfo_new(10000, 0, 0, 0);
    seL4_Call(ZX_THREAD_SYSCALL_SLOT, tag);
    err = seL4_GetMR(0);
    assert(err == ZX_ERR_BAD_SYSCALL);

    /* Handle close tests */
    assert(zx_handle_close(ZX_HANDLE_INVALID) == ZX_OK);
    assert(zx_handle_close(1) == ZX_ERR_BAD_HANDLE);

    /* Handle replace test. Each replace returns a new handle value */
    zx_handle_t thrd_handle2 = 0;
    err = zx_handle_replace(thrd_handle, ZX_RIGHT_SAME_RIGHTS, &thrd_handle2);
    printf("zx_handle_replace returned %d, thrd_handle2 %u\n", err, thrd_handle2);
    assert(!zx_handle_replace(thrd_handle2, ZX_RIGHT_SAME_RIGHTS, &thrd_handle));
    printf("zx_handle_replace returned %d, thrd_handle %u\n", err, thrd_handle);

    /* Fifo test */
    printf("Test creation of a fifo\n");
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

    /* Create a socket */
    assert(!zx_socket_create(ZX_SOCKET_STREAM, &socket0, &socket1));
    const char *sock_msg = "Hello thread!";
    assert(!zx_socket_write(socket0, 0, sock_msg, strlen(sock_msg), NULL));

    /* Create a test thread */
    printf("Creating a test thread\n");
    zx_handle_t new_thrd;
    const char *name = "thrd2";
    assert(!zx_thread_create(proc_handle, name, strlen(name), 0, &new_thrd));

    /* Create a stack vmo for the thread */
    printf("Creating a stack vmo\n");
    zx_handle_t stack_vmo;
    assert(!zx_vmo_create(CHANNEL_BUF_SIZE * 2, 0, &stack_vmo));
    uint64_t stack_size;
    assert(!zx_vmo_get_size(stack_vmo, &stack_size));
    printf("Size of stack vmo: %lu\n", stack_size);

    /* Map the stack vmo in our vmar */
    uint32_t map_flags = ZX_VM_FLAG_PERM_READ | ZX_VM_FLAG_PERM_WRITE;
    uint64_t mapped_addr;
    assert(!zx_vmar_map(vmar_handle, 0, stack_vmo, 0, stack_size, map_flags, &mapped_addr));
    printf("Stack mapped in vmar at %lx\n", mapped_addr);

    /* Align the stack & start thread */
    uintptr_t stack_addr = compute_initial_stack_pointer(mapped_addr, stack_size);
    printf("Starting thread...\n");
    assert(!zx_thread_start(new_thrd, (uintptr_t)thread_entry, stack_addr, 9, 6));

    /* Create test endpoint */
    assert(!zx_endpoint_create(rsrc_handle, TEST_EP_ID, 0, &ep_handle));
    printf("Created endpoint, handle %u\n", ep_handle);

    /* Mint a cap to each thread's cspace */
    assert(!zx_endpoint_mint_cap(ep_handle, thrd_handle, TEST_EP_SLOT, 1, TEST_EP_RIGHTS));
    assert(!zx_endpoint_mint_cap(ep_handle, new_thrd, TEST_EP_SLOT, 2, TEST_EP_RIGHTS));
    printf("Minted endpoint caps.\n");

    /* Do a wait to sync with other thread */
    assert(!zx_object_wait_one(event_handle, ZX_USER_SIGNAL_2, ZX_TIME_INFINITE, NULL));
    printf("Main thread woke from wait one.\n");

    /* Send a message to the other thread */
    printf("Sending msg to thread\n");
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, TEST_EP_MSG);
    seL4_Send(TEST_EP_SLOT, tag);

    printf("Main thread sleeping for 2 seconds.\n");
    zx_time_t deadline;
    deadline = zx_deadline_after(ZX_SEC(2));
    zx_nanosleep(deadline);
    printf("Main thread woke from sleep.\n");

    /* Delete endpoint caps from threads */
    assert(!zx_endpoint_delete_cap(ep_handle, thrd_handle, TEST_EP_SLOT));
    assert(!zx_endpoint_delete_cap(ep_handle, new_thrd, TEST_EP_SLOT));
    printf("Deleted endpoint caps.\n");

    /* Ping pong messages with test thread using channel */
    assert(!zx_channel_create(0, &ch0, &ch1));
    char writebuf[CHANNEL_BUF_SIZE];
    char readbuf[CHANNEL_BUF_SIZE];
    srand(6123129);

    assert(!zx_object_signal(event_handle, 0u, ZX_USER_SIGNAL_3));
    for (int i = 0; i < NUM_CHANNEL_RUNS; ++i) {
        printf("Channel test with thread %d\n", i);
        for (int j = 0; j < CHANNEL_BUF_SIZE; j += sizeof(int)) {
            int *val = (int *)&writebuf[j];
            *val = rand();
        }
        assert(!zx_channel_write(ch0, 0, writebuf, CHANNEL_BUF_SIZE, NULL, 0));
        assert(!zx_object_wait_one(ch0, ZX_CHANNEL_READABLE,  ZX_TIME_INFINITE, NULL));
        assert(!zx_channel_read(ch0, 0, readbuf, NULL, CHANNEL_BUF_SIZE, 0, NULL, NULL));
        assert(!memcmp(writebuf, readbuf, CHANNEL_BUF_SIZE));
        //zx_nanosleep(zx_deadline_after(ZX_MSEC(200)));
    }

    /* Kill other thread */
    assert(!zx_task_kill(new_thrd));

    /* Create a dummy process with miniprocess library */
    printf("Creating dummy process...\n");
    zx_handle_t dummy_proc, dummy_thrd, dummy_vmar;
    assert(!zx_process_create(job_handle, "minipr", 6, 0,  &dummy_proc, &dummy_vmar));
    assert(!zx_thread_create(dummy_proc, "minith", 6, 0, &dummy_thrd));
    assert(!start_mini_process_etc(dummy_proc, dummy_thrd, dummy_vmar, event_handle, NULL));
    printf("Waiting for dummy process to fault...\n");
    assert(!zx_object_wait_one(dummy_proc, ZX_TASK_TERMINATED, ZX_TIME_INFINITE, NULL));

    /* Run an actual process */
    printf("Running hello world / ping process\n");
    run_hello_world();

    /* After exiting, all resources will be destroyed, since no processes are running */
    printf("Zircon test exiting! This will kill the root job.\n");
    zx_process_exit(0);

    printf("We shouldn't get here!\n");
    return 0;
}
