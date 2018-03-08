#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <zircon/types.h>
}

#include "object.h"
#include "process.h"

#define MAX_NUM_HANDLES 8192

#define HANDLE_MASK     0x3ffffu
#define DEFAULT_MASK    0x40000u

class Handle : public Listable {
public:
    Handle(ZxProcess *owner, ZxObject *obj, zx_rights_t rights, uint32_t value) :
        owner_{owner}, obj_{obj}, rights_{rights_}, base_value_{value} {}

    ZxProcess *get_owner() const { return owner_; }

private:
    ZxProcess *owner_;
    ZxObject *obj_;
    const zx_rights_t rights_;
    const uint32_t base_value_;
};

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
