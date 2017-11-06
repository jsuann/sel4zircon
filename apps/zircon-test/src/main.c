/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4utils/process.h>

#include "syscall.h"

/* constants */
#define EP_CPTR SEL4UTILS_FIRST_FREE // where the cap for the endpoint was placed.
#define MSG_DATA 0x2 //  arbitrary data to send

int main(int argc, char **argv) {
    seL4_MessageInfo_t tag;
    //seL4_Word msg;

    printf(">=== Zircon Test ===\n");

    // test handle acquire syscall
    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, MSG_DATA);
    tag = seL4_Call(EP_CPTR, tag);

    zx_handle_t handle = seL4_GetMR(0);
    printf(">received handle! %u\n", handle);

    tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, 0xff);
    tag = seL4_Call(handle, tag);

    zx_null(handle);

    return 0;
}
