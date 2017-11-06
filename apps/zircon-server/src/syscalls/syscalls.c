#include "syscalls.h"

zx_syscall_func sys_table[NUM_SYSCALLS] = {
    sys_null,
    sys_handle_close,
    sys_handle_replace,
    sys_handle_duplicate
};

void sys_null(uint32_t handle)
{
    printf("Got null syscall! %u\n", handle);
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
    seL4_Reply(tag);
}
