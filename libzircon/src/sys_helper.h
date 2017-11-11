#pragma once

#include <sel4/sel4.h>
#include <sel4utils/process.h>

#include "sys_def.h"

#define DEFAULT_HANDLE_CPTR SEL4UTILS_FIRST_FREE

/*
 * Macros for performing syscalls
 */

#define ZX_SYSCALL_SEND(handle, syscall, n, ...) \
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(syscall, 0, 0, n); \
    ZX_SET_ARGS(n, __VA_ARGS__) \
    seL4_Call(handle, tag)

#define ZX_SYSCALL_SEND_WITH_CAPS(handle, syscall, num_caps, n, ...) \
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(syscall, 0, num_caps, n); \
    ZX_SET_CAPS(num_caps, n, __VA_ARGS__) \
    seL4_Call(handle, tag)

#define ZX_SET_ARGS(n, ...) ZX_SET_ARGS_##n(0, __VA_ARGS__)

#define ZX_SET_ARGS_0(...)
#define ZX_SET_ARGS_1(x, arg)       seL4_SetMR(x, arg);
#define ZX_SET_ARGS_2(x, arg, ...)  seL4_SetMR(x, arg); ZX_SET_ARGS_1(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_3(x, arg, ...)  seL4_SetMR(x, arg); ZX_SET_ARGS_2(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_4(x, arg, ...)  seL4_SetMR(x, arg); ZX_SET_ARGS_3(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_5(x, arg, ...)  seL4_SetMR(x, arg); ZX_SET_ARGS_4(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_6(x, arg, ...)  seL4_SetMR(x, arg); ZX_SET_ARGS_5(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_7(x, arg, ...)  seL4_SetMR(x, arg); ZX_SET_ARGS_6(INC(x), __VA_ARGS__)
#define ZX_SET_ARGS_8(x, arg, ...)  seL4_SetMR(x, arg); ZX_SET_ARGS_7(INC(x), __VA_ARGS__)


#define ZX_SET_CAPS(num_caps, n, ...)   ZX_SET_CAPS_##num_caps(0, n, __VA_ARGS__)

#define ZX_SET_CAPS_1(x, n, cap, ...)   seL4_SetCap(x, cap); ZX_SET_ARGS(n, __VA_ARGS__)
#define ZX_SET_CAPS_2(x, n, cap, ...)   seL4_SetCap(x, cap); ZX_SET_CAPS_1(INC(x), n, __VA_ARGS__)


#define INC(x)  INC_##x
#define INC_0   1
#define INC_1   2
#define INC_2   3
#define INC_3   4
#define INC_4   5
#define INC_5   6
#define INC_6   7
#define INC_7   8
