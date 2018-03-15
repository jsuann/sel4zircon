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

class ZxThread final : public ZxObject, public Listable<ZxThread> {
public:
    ZxThread() {
        memset(name_, 0, ZX_MAX_NAME_LEN);
    }

    ~ZxThread() final {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_THREAD; }
private:
    /* Starting register values */
    uintptr_t user_entry_ = 0;
    uintptr_t user_sp_ = 0;
    uintptr_t user_arg1_ = 0;
    uintptr_t user_arg2_ = 0;

    /* State */
    /* Exception port */

    char name_[ZX_MAX_NAME_LEN];

    struct seL4_ThreadData {

    };
};
