#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include "syscalls.h"

void sys_undefined(seL4_MessageInfo_t tag, uint64_t badge)
{
    /* Get the syscall number */
    seL4_Word syscall = seL4_MessageInfo_get_label(tag);

    /* TODO Determine the calling process */
    printf("Received unimplemented syscall: %lu\n", syscall);
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}
