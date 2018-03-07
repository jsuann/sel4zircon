#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <zircon/types.h>
}

class ZxObject {
public:
    virtual zx_obj_type_t get_object_type() const { return ZX_OBJ_TYPE_NONE; }
    ZxObject(zx_koid_t koid) : koid_{koid},  handle_count_{0}, signals_{0} {}
    virtual ~ZxObject();

    zx_koid_t get_koid() const { return koid_; }

    void increment_handle_count() {
        ++handle_count_;
    }

    bool decrement_handle_count() {
        --handle_count_;
        return handle_count_ == 0u;
    }

    uint32_t current_handle_count() const {
        return handle_count_;
    }

private:
    const zx_koid_t koid_;
    uint32_t handle_count_;
    zx_signals_t signals_;
};
