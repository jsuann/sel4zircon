#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <zircon/types.h>

#include "object.h"

#define MAX_NUM_HANDLES 8192

#define HANDLE_MASK     0x3ffffu
#define DEFAULT_MASK    0x40000u

typedef struct zir_handle {
    // ptr to owning process
    zir_object_t *process;
    // ptr to object referred to
    zir_object_t *object;
    // rights of handle
    zx_rights_t rights;
    // map CPtr to zx_handle_t
    zx_handle_t handle_cap;
    // linked list for processes
    struct zir_handle *next;
    struct zir_handle *prev;
} zir_handle_t;

extern zir_handle_t *handle_arena;

/* initialise handle arena */
int init_handle_arena(vspace_t *vspace);

uint32_t allocate_handle(void *process, uint32_t rights, void *object);

void free_handle(uint32_t val);

/* helper funcs */
static inline void *get_handle_process(uint32_t handle)
{
    return handle_arena[handle].process;
}

static inline uint32_t get_handle_rights(uint32_t handle)
{
    return handle_arena[handle].rights;
}

static inline void *get_handle_object(uint32_t handle)
{
    return handle_arena[handle].object;
}

static inline int is_default_handle(uint32_t handle)
{
    return (handle & DEFAULT_MASK);
}
