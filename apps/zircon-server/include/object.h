#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <zircon/types.h>

enum object_type {
    ZIR_INVALID,
    ZIR_PROCESS,
    ZIR_VMAR,
    ZIR_VMO,
    ZIR_CHANNEL
};

typedef struct zir_object {
    // TODO koid
    uint32_t handle_count;
    enum object_type type;
    // TODO signals, observers?
} zir_object_t;

static inline void init_zir_object(zir_object_t *obj, enum object_type type)
{
    obj->handle_count = 0;
    obj->type = type;
}
