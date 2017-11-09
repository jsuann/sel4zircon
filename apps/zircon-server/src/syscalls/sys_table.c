#include "syscalls.h"

zx_syscall_func sys_table[NUM_SYSCALLS] = {
    sys_null,
    sys_handle_close,
    sys_handle_replace,
    sys_handle_duplicate,
    sys_channel_create,
    sys_channel_read,
    sys_channel_write,
    sys_channel_call,
    sys_test_0,
    sys_test_1,
    sys_test_2,
    sys_test_3,
    sys_test_4,
    sys_test_5,
    sys_test_6,
    sys_test_7,
    sys_test_8,
};
