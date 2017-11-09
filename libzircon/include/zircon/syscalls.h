#pragma once

#include <autoconf.h>

#include "types.h"
#include "errors.h"

void zx_null(zx_handle_t handle);

zx_status_t zx_handle_replace(zx_handle_t handle, zx_rights_t rights, zx_handle_t* out);

zx_status_t syscall_test_0(void);
zx_status_t syscall_test_1(int a);
zx_status_t syscall_test_2(int a, int b); 
zx_status_t syscall_test_3(int a, int b, int c); 
zx_status_t syscall_test_4(int a, int b, int c, int d); 
zx_status_t syscall_test_5(int a, int b, int c, int d, int e); 
zx_status_t syscall_test_6(int a, int b, int c, int d, int e, int f); 
zx_status_t syscall_test_7(int a, int b, int c, int d, int e, int f, int g); 
zx_status_t syscall_test_8(int a, int b, int c, int d, int e, int f, int g, int h); 
