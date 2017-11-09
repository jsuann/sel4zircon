#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <sel4/sel4.h>

typedef void (*zx_syscall_func)(seL4_MessageInfo_t tag, uint32_t handle);

// generated file containing server syscall defs
#include "syscall_defs.h"

extern zx_syscall_func sys_table[];

#define DO_SYSCALL(x, tag, handle)   sys_table[x](tag, handle)

// generic reply function
static inline void
sys_reply(int32_t res)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, res);
    seL4_Reply(tag);
}
