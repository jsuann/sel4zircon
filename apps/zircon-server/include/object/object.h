#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <zircon/types.h>
}

#include "../zxcpp/new.h"
#include "handle.h"

class Handle;

class ZxObject {
public:
    ZxObject();
    virtual ~ZxObject() {}

    virtual zx_obj_type_t get_object_type() const { return ZX_OBJ_TYPE_NONE; }

    zx_koid_t get_koid() const { return koid_; }

    Handle *create_handle(zx_rights_t rights) {
        Handle *h = allocate_handle(this, rights);
        if (h != NULL) {
            ++handle_count_;
        }
        return h;
    }

    bool destroy_handle(Handle *h) {
        free_handle(h);
        --handle_count_;
        return handle_count_ == 0u;
    }

    uint32_t current_handle_count() const {
        return handle_count_;
    }

    void print_object_info() const {
        dprintf(INFO, "%lu %u %u\n", koid_, handle_count_, signals_);
    }

private:
    const zx_koid_t koid_;
    uint32_t handle_count_;
    zx_signals_t signals_;
};

/* generic object allocation */
template <typename T, typename ... U>
T *allocate_object(U ... args);

template <typename T>
void free_object(T *obj);

void destroy_object(ZxObject *obj);

template <typename T>
bool is_object_type(ZxObject *obj);
