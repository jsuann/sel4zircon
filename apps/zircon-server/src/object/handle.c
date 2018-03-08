/*
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <vspace/vspace.h>

#include "handle.h"

zir_handle_t *handle_arena;

UNUSED uint32_t vaddr_to_handle_val(void *vaddr)
{
    uintptr_t val = (uintptr_t)vaddr;
    val -= (uintptr_t)&handle_arena[0];
    return (uint32_t)val;
}

int init_handle_arena(vspace_t *vspace)
{
    int num_handle_pages = ((MAX_NUM_HANDLES * sizeof(zir_handle_t))+BIT(seL4_PageBits)-1)/BIT(seL4_PageBits);
    handle_arena = (zir_handle_t *)vspace_new_pages(vspace, seL4_AllRights, num_handle_pages, seL4_PageBits);
    assert(handle_arena != NULL);

    printf("arena ptr: %p, handle size: %lu\n", handle_arena, sizeof(zir_handle_t));
    printf("%lu %d %lu\n", sizeof(zir_handle_t)*MAX_NUM_HANDLES, num_handle_pages, num_handle_pages*BIT(seL4_PageBits));

    for (int i = 0; i < MAX_NUM_HANDLES; i++) {
        handle_arena[i].process = NULL;
        // use right to store next up handle
        handle_arena[i].rights = i+1;
        // point to next handle for allocation
        handle_arena[i].object = NULL;
        handle_arena[i].handle_cap = 0;
        handle_arena[i].next = NULL;
        handle_arena[i].prev = NULL;
    }

    handle_arena[MAX_NUM_HANDLES-1].rights = 0;

    return 0;
}

uint32_t allocate_handle(void *process, uint32_t rights, void *object)
{
    // check a handle is available
    if (handle_arena[0].rights == 0) {
        printf("out of handles!\n");
        return ZX_HANDLE_INVALID;
    }

    // allocate handle
    uint32_t val = handle_arena[0].rights;
    handle_arena[0].rights = handle_arena[val].rights;

    assert(handle_arena[val].object == NULL);

    handle_arena[val].process = process;
    handle_arena[val].object = object;
    handle_arena[val].rights = rights;
    
    return val;
}

void free_handle(uint32_t val)
{
    zir_handle_t *handle = &handle_arena[val];
    // invalidate handle
    handle->process = NULL;
    handle->object = NULL;
   
    // put handle at front of stack
    handle->rights = handle_arena[0].rights;
    handle_arena[0].rights = val;
}
*/
