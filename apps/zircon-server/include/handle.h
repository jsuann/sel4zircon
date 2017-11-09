#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <zircon/types.h>

#define MAX_NUM_HANDLES 8192

typedef struct handle {
    void *process;
    void *object;
    uint32_t rights;
    uint32_t handle_cap;
    // linked list for processes
    struct handle *next;
    struct handle *prev;
} handle_t;

extern handle_t *handle_arena;

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
