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

struct CookieJar {
    zx_koid_t scope_ = ZX_KOID_INVALID;
    uint64_t cookie_ = 0u;
};

class ZxObject {
public:
    ZxObject(zx_signals_t signals = 0u);
    virtual ~ZxObject() {}

    virtual zx_obj_type_t get_object_type() const {
        assert(!"Object type wasn't set!");
        return ZX_OBJ_TYPE_NONE;
    }

    /* All objects need to define a destroy/cleanup function */
    virtual void destroy() { assert(!"Attempt to destroy base object!"); }

    /* Check that object has no more references & is safe to destroy */
    virtual bool can_destroy() {
        /* For most objects, this is just when handle count reaches zero.
           This must be overriden if there are additional refs to account for. */
        return (handle_count_ == 0);
    }

    /* Overridden by ZxObjectWaitable if object can have state waiters */
    virtual bool has_state_tracker() const { return false; }
    virtual void update_waiters(zx_signals_t signals) {}
    virtual void cancel_waiters(Handle *h) {}

    /* Override if object can store cookie */
    virtual CookieJar *get_cookie_jar() { return NULL; }

    virtual zx_status_t user_signal(uint32_t clear_mask, uint32_t set_mask,
            bool peer);

    /* Helper for when can_destroy is overriden */
    bool zero_handles() const { return (handle_count_ == 0); }

    zx_koid_t get_koid() const { return koid_; }

    Handle *create_handle(zx_rights_t rights) {
        Handle *h = allocate_handle(this, rights);

        if (h != NULL) {
            ++handle_count_;
        }

        return h;
    }

    void destroy_handle(Handle *h) {
        cancel_waiters(h);
        free_handle(h);
        --handle_count_;
    }

    uint32_t current_handle_count() const {
        return handle_count_;
    }

    void print_object_info() const {
        dprintf(INFO, "%lu %u %u\n", koid_, handle_count_, signals_);
    }

    zx_signals_t get_signals() const {
        return signals_;
    }

    void update_state(zx_signals_t clear_mask, zx_signals_t set_mask) {
        zx_signals_t prev = signals_;
        signals_ &= ~clear_mask;
        signals_ |= set_mask;

        if (prev != signals_) {
            update_waiters(signals_);
        }
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

template <typename T>
bool is_object_type(ZxObject *obj);

/* Combine destroying of handle & object */
void destroy_handle_maybe_object(Handle *h);

/* Helper for new object creation */
template <typename T>
Handle *create_handle_default_rights(ZxObject *obj);
