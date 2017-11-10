#pragma once

#include <autoconf.h>

#include "types.h"
#include "errors.h"

// TODO should have void arg
void zx_null(zx_handle_t handle);

/* Handle syscalls */
zx_status_t zx_handle_replace(zx_handle_t handle, zx_rights_t rights, zx_handle_t* out);

/* Process syscalls */
zx_status_t zx_process_create(zx_handle_t job,
                              const char* name, uint32_t name_len,
                              uint32_t options,
                              zx_handle_t* proc_handle, zx_handle_t* vmar_handle);

zx_status_t zx_process_start(zx_handle_t process, zx_handle_t thread,
                            uintptr_t entry, uintptr_t stack,
                            zx_handle_t arg1, uintptr_t arg2);

void zx_process_exit(int ret_code);

/* Test syscalls */
zx_status_t zx_syscall_test_0(void);
zx_status_t zx_syscall_test_1(int a);
zx_status_t zx_syscall_test_2(int a, int b);
zx_status_t zx_syscall_test_3(int a, int b, int c);
zx_status_t zx_syscall_test_4(int a, int b, int c, int d);
zx_status_t zx_syscall_test_5(int a, int b, int c, int d, int e);
zx_status_t zx_syscall_test_6(int a, int b, int c, int d, int e, int f);
zx_status_t zx_syscall_test_7(int a, int b, int c, int d, int e, int f, int g);
zx_status_t zx_syscall_test_8(int a, int b, int c, int d, int e, int f, int g, int h);
