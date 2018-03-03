#pragma once

/* Keeping for reference */
/*
#define ZX_SYSCALL_SEND_WITH_CAPS(handle, syscall, num_caps, n, ...) \
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(syscall, 0, num_caps, n); \
    ZX_SET_CAPS(num_caps, n, __VA_ARGS__) \
    seL4_Call(handle, tag)

#define ZX_SET_CAPS(num_caps, n, ...)   ZX_SET_CAPS_##num_caps(0, n, __VA_ARGS__)

#define ZX_SET_CAPS_1(x, n, cap, ...)   seL4_SetCap(x, cap); ZX_SET_ARGS(n, __VA_ARGS__)
#define ZX_SET_CAPS_2(x, n, cap, ...)   seL4_SetCap(x, cap); ZX_SET_CAPS_1(INC(x), n, __VA_ARGS__)
*/
