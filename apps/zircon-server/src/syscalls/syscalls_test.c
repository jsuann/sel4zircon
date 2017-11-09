#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "syscalls.h"

#define DO_TEST_SYSCALL(n) \
    printf("sys test %d\n", n); \
    if (seL4_MessageInfo_get_length(tag) != n) { \
        sys_reply(-1); \
        return; \
    } \
    printf("Args:"); \
    for (int i = 0; i < n; i++) { \
        printf(" %lu", seL4_GetMR(i)); \
    } \
    printf("\n"); \
    sys_reply(0);

void sys_test_0(seL4_MessageInfo_t tag, uint32_t handle)
{
    DO_TEST_SYSCALL(0);
}

void sys_test_1(seL4_MessageInfo_t tag, uint32_t handle)
{
    DO_TEST_SYSCALL(1);
}

void sys_test_2(seL4_MessageInfo_t tag, uint32_t handle)
{
    DO_TEST_SYSCALL(2);
}

void sys_test_3(seL4_MessageInfo_t tag, uint32_t handle)
{
    DO_TEST_SYSCALL(3);
}

void sys_test_4(seL4_MessageInfo_t tag, uint32_t handle)
{
    DO_TEST_SYSCALL(4);
}

void sys_test_5(seL4_MessageInfo_t tag, uint32_t handle)
{
    DO_TEST_SYSCALL(5);
}

void sys_test_6(seL4_MessageInfo_t tag, uint32_t handle)
{
    DO_TEST_SYSCALL(6);
}

void sys_test_7(seL4_MessageInfo_t tag, uint32_t handle)
{
    DO_TEST_SYSCALL(7);
}

void sys_test_8(seL4_MessageInfo_t tag, uint32_t handle)
{
    DO_TEST_SYSCALL(8);
}
