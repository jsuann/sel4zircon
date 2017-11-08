#include "syscalls.h"

zx_syscall_func sys_table[NUM_SYSCALLS] = {
    // generated file containing elements of table
    #include "sys_table.h"
};

void sys_null(seL4_MessageInfo_t tag, uint32_t handle)
{
    printf("Got null syscall! %u\n", handle);
    printf("Num args: %lu\n", seL4_MessageInfo_get_length(tag));
    tag = seL4_MessageInfo_new(0, 0, 0, 0);
    seL4_Reply(tag);
}
