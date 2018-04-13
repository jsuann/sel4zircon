#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"

/* Test syscalls return sum of args */
#define DO_TEST_SYSCALL(n) \
    dprintf(INFO, "Got %s\n", __FUNCTION__); \
    SYS_CHECK_NUM_ARGS(tag, n); \
    uint64_t ret = 0; \
    for (int i = 0; i < n; i++) \
        ret += seL4_GetMR(i); \
    sys_reply(ret);

void sys_syscall_test_0(seL4_MessageInfo_t tag, uint64_t badge)
{
    dprintf(INFO, "Got %s\n", __FUNCTION__);
    SYS_CHECK_NUM_ARGS(tag, 0);
    sys_reply(0);
}

void sys_syscall_test_1(seL4_MessageInfo_t tag, uint64_t badge)
{
    DO_TEST_SYSCALL(1);
}

void sys_syscall_test_2(seL4_MessageInfo_t tag, uint64_t badge)
{
    DO_TEST_SYSCALL(2);
}

void sys_syscall_test_3(seL4_MessageInfo_t tag, uint64_t badge)
{
    DO_TEST_SYSCALL(3);
}

void sys_syscall_test_4(seL4_MessageInfo_t tag, uint64_t badge)
{
    DO_TEST_SYSCALL(4);
}

void sys_syscall_test_5(seL4_MessageInfo_t tag, uint64_t badge)
{
    DO_TEST_SYSCALL(5);
}

void sys_syscall_test_6(seL4_MessageInfo_t tag, uint64_t badge)
{
    DO_TEST_SYSCALL(6);
}

void sys_syscall_test_7(seL4_MessageInfo_t tag, uint64_t badge)
{
    DO_TEST_SYSCALL(7);
}

void sys_syscall_test_8(seL4_MessageInfo_t tag, uint64_t badge)
{
    DO_TEST_SYSCALL(8);
}

void sys_syscall_test_wrapper(seL4_MessageInfo_t tag, uint64_t badge)
{
    DO_TEST_SYSCALL(3);
}
