#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"

namespace SysTests {
/* Test syscalls return sum of args */
#define DO_TEST_SYSCALL(n)  DO_TEST_SYSCALL_##n
#define DO_TEST_SYSCALL_1   seL4_GetMR(0)
#define DO_TEST_SYSCALL_2   seL4_GetMR(1) + DO_TEST_SYSCALL_1
#define DO_TEST_SYSCALL_3   seL4_GetMR(2) + DO_TEST_SYSCALL_2
#define DO_TEST_SYSCALL_4   seL4_GetMR(3) + DO_TEST_SYSCALL_3
#define DO_TEST_SYSCALL_5   seL4_GetMR(4) + DO_TEST_SYSCALL_4
#define DO_TEST_SYSCALL_6   seL4_GetMR(5) + DO_TEST_SYSCALL_5
#define DO_TEST_SYSCALL_7   seL4_GetMR(6) + DO_TEST_SYSCALL_6
#define DO_TEST_SYSCALL_8   seL4_GetMR(7) + DO_TEST_SYSCALL_7
}

uint64_t sys_syscall_test_0(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 0);
    return 0;
}

uint64_t sys_syscall_test_1(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysTests;
    SYS_CHECK_NUM_ARGS(tag, 1);
    return DO_TEST_SYSCALL(1);
}

uint64_t sys_syscall_test_2(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysTests;
    SYS_CHECK_NUM_ARGS(tag, 2);
    return DO_TEST_SYSCALL(2);
}

uint64_t sys_syscall_test_3(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysTests;
    SYS_CHECK_NUM_ARGS(tag, 3);
    return DO_TEST_SYSCALL(3);
}

uint64_t sys_syscall_test_4(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysTests;
    SYS_CHECK_NUM_ARGS(tag, 4);
    return DO_TEST_SYSCALL(4);
}

uint64_t sys_syscall_test_5(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysTests;
    SYS_CHECK_NUM_ARGS(tag, 5);
    return DO_TEST_SYSCALL(5);
}

uint64_t sys_syscall_test_6(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysTests;
    SYS_CHECK_NUM_ARGS(tag, 6);
    return DO_TEST_SYSCALL(6);
}

uint64_t sys_syscall_test_7(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysTests;
    SYS_CHECK_NUM_ARGS(tag, 7);
    return DO_TEST_SYSCALL(7);
}

uint64_t sys_syscall_test_8(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysTests;
    SYS_CHECK_NUM_ARGS(tag, 8);
    return DO_TEST_SYSCALL(8);
}

uint64_t sys_syscall_test_wrapper(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysTests;
    SYS_CHECK_NUM_ARGS(tag, 3);
    return DO_TEST_SYSCALL(3);
}
