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

class Handle : public Listable<Handle> {
public:
    Handle(ZxObject *obj, zx_rights_t rights, uint32_t base_value) :
            obj_{obj}, rights_{rights}, base_value_{base_value} {}

    ZxObject *get_object() const { return obj_; }
    const zx_rights_t get_rights() const { return rights_; }
    const uint32_t get_value() const { return base_value_; }

    bool has_rights(zx_rights_t desired) const {
        return (rights_ & desired) == desired;
    }

private:
    ZxObject *obj_;
    const zx_rights_t rights_;
    const uint32_t base_value_;
};

void init_handle_table(vspace_t *vspace);
Handle *allocate_handle(ZxObject *obj, zx_rights_t rights);
void free_handle(Handle *h);
Handle *base_value_to_addr(uint32_t base_value);
