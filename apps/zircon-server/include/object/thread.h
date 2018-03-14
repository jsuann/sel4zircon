#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

#include "listable.h"
#include "object.h"

class ZxThread final : public ZxObject, public Listable<ZxProcess> {
public:
    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_PROCESS; }
private:
    /* TODO */
    uintptr_t user_entry_ = 0;
};
