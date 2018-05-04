#pragma once

#include <autoconf.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

/* generic reply function */
static inline void sys_reply(seL4_Word res)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, res);
    seL4_Reply(tag);
}

/* Check that syscall has correct num args */
#define SYS_CHECK_NUM_ARGS(tag, n) \
    do { \
        if (seL4_MessageInfo_get_length(tag) != n) { \
            return sys_reply(ZX_ERR_INVALID_ARGS); \
        } \
    } while (0)

/* Reply & return if a function returns error code */
#define SYS_RET_IF_ERR(err) \
    do { \
        if (err != ZX_OK) { \
            return sys_reply(err); \
        } \
    } while (0)
