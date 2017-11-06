#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <sel4/sel4.h>

typedef void (*zx_syscall_func)(uint32_t handle);

// generated file containing server syscall defs
#include "syscall_defs.h"

extern zx_syscall_func sys_table[];

#define DO_SYSCALL(x, handle)   sys_table[x](handle)
