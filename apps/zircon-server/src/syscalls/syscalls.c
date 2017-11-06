#include "syscalls.h"

zx_syscall_func sys_table[NUM_SYSCALLS] = {
    // generated file containing element of table
    #include "sys_table.h"
};

void sys_null(uint32_t handle)
{
    printf("Got null syscall! %u\n", handle);
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
    seL4_Reply(tag);
}
