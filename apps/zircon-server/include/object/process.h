#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <sel4utils/process.h>
#include <zircon/types.h>
}

#include "object.h"

class ZxProcess final : public ZxObject {
public:
    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_PROCESS; }
private:
};
