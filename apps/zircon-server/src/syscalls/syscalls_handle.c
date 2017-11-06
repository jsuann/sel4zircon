#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "syscalls.h"
#include "handle.h"

void sys_handle_close(uint32_t handle)
{
}

void sys_handle_replace(uint32_t handle)
{
    printf("sys handle replace: %u\n", handle);
}

void sys_handle_duplicate(uint32_t handle)
{
    printf("sys handle duplicate: %u\n", handle);
}
