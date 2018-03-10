#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <zircon/types.h>
}

#include "../zxcpp/new.h"
#include "listable.h"

class ZxObject;
class ZxProcess;

class Handle : public Listable<Handle> {
public:
    Handle(ZxObject *obj, zx_rights_t rights, uint32_t base_value) :
        owner_{NULL}, obj_{obj}, rights_{rights}, base_value_{base_value} {}

    /* override for listable */
    ZxObject *get_owner() const override { return (ZxObject *)owner_; }
    void set_owner(ZxObject *o) override { owner_ = (ZxProcess *)o; }

    ZxObject *get_object() const { return obj_; }
    const zx_rights_t get_rights() const { return rights_; }
    const uint32_t get_value() const { return base_value_; }

private:
    ZxProcess *owner_;
    ZxObject *obj_;
    const zx_rights_t rights_;
    const uint32_t base_value_;
};

uint32_t get_new_base_value(void *p);

/* FIXME handles should use arena allocation */
Handle *allocate_handle(ZxObject *obj, zx_rights_t rights)
{
    void *p = malloc(sizeof(Handle));
    if (p == NULL) {
        return NULL;
    }
    Handle *h = new (p) Handle(obj, rights, get_new_base_value(p));
    return h;
}

void free_handle(Handle *h)
{
    delete h;
}
