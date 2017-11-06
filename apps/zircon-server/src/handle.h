
#pragma once

#define MAX_NUM_HANDLES 32768 //(256 * 1024)

#define ZX_HANDLE_INVALID (0)

typedef struct handle {
    void *process;
    void *object;
    uint32_t rights;
    uint32_t handle_cap;
} handle_t;

handle_t *handle_arena;

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
