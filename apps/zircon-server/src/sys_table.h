#include <stdio.h>

typedef void (*zx_syscall_func)(uint32_t handle);

#define NUM_SYSCALLS    4

void sys_null(uint32_t handle)
{
    printf("Got null syscall! %u\n", handle);
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 0);
    seL4_Reply(tag);
}

void sys_handle_close(uint32_t handle)
{
}

void sys_handle_replace(uint32_t handle)
{
    printf("sys handle replace: %u\n", handle);
}

void sys_handle_duplicate(uint32_t handle)
{
}

zx_syscall_func sys_table[NUM_SYSCALLS] = {
    sys_null,
    sys_handle_close,
    sys_handle_replace,
    sys_handle_duplicate
};

#define DO_SYSCALL(x, handle)   sys_table[x](handle);
