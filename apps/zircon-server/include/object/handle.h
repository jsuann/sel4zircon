#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <zircon/types.h>
}

#include "listable.h"

#define MAX_NUM_HANDLES 8192

#define HANDLE_MASK     0x3ffffu
#define DEFAULT_MASK    0x40000u

class ZxObject;
class ZxProcess;

class Handle : public Listable {
public:
    Handle(ZxProcess *owner, ZxObject *obj, zx_rights_t rights, uint32_t value) :
        owner_{owner}, obj_{obj}, rights_{rights_}, base_value_{value} {}

    /* override for listable */
    ZxObject *get_owner() const override { return (ZxObject *)owner_; }

    //ZxProcess *get_owner() const override { return owner_; }
    ZxObject *get_object() const { return obj_; }
    const zx_rights_t get_rights() const { return rights_; }
    const uint32_t get_value() const { return base_value_; }

private:
    ZxProcess *owner_;
    ZxObject *obj_;
    const zx_rights_t rights_;
    const uint32_t base_value_;
};

//extern zir_handle_t *handle_arena;

/* initialise handle arena */
//int init_handle_arena(vspace_t *vspace);

//uint32_t allocate_handle(void *process, uint32_t rights, void *object);

//void free_handle(uint32_t val);
