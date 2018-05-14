#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "object/vmar.h"
#include "sys_helpers.h"

uint64_t sys_vmar_allocate(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 6);
    zx_handle_t parent_vmar_handle = seL4_GetMR(0);
    size_t offset = seL4_GetMR(1);
    size_t size = seL4_GetMR(2);
    uint32_t map_flags = seL4_GetMR(3);
    uintptr_t user_child_vmar = seL4_GetMR(4);
    uintptr_t user_child_addr = seL4_GetMR(5);

    zx_handle_t *child_vmar;
    uintptr_t *child_addr;
}
