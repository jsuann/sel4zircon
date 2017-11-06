#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <sel4/sel4.h>

typedef void (*zx_syscall_func)(uint32_t handle);

#define NUM_SYSCALLS    4

void sys_null(uint32_t handle);
void sys_handle_close(uint32_t handle);
void sys_handle_replace(uint32_t handle);
void sys_handle_duplicate(uint32_t handle);

extern zx_syscall_func sys_table[];

#define DO_SYSCALL(x, handle)   sys_table[x](handle);
