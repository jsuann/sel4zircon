#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
#include <zircon/syscalls/resource.h>
}

#include "object.h"

class ZxProcess;

class ZxResource final : public ZxObjectWaitable {
public:
    ZxResource(uint32_t kind, uint64_t low, uint64_t high) :
           kind_{kind}, low_{low}, high_{high}  {}
    ~ZxResource() final {}

    void destroy() override {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_RESOURCE; }
    CookieJar* get_cookie_jar() override { return &cookie_jar_; }

    uint32_t get_kind() const { return kind_; }
    void get_range(uint64_t* low, uint64_t* high) { *low = low_, *high = high_; }

private:
    const uint32_t kind_;
    const uint64_t low_;
    const uint64_t high_;

    CookieJar cookie_jar_;
};

void init_root_resource();
ZxResource *get_root_resource();

/* Validates a resource based on type */
zx_status_t validate_resource(ZxProcess *proc, zx_handle_t handle, uint32_t kind);

/* Validates a resource based on type and low/high range */
zx_status_t validate_ranged_resource(ZxProcess *proc, zx_handle_t handle,
        uint32_t kind, uint64_t low, uint64_t high);

/* Validates mapping an MMIO range based on a resource handle */
static inline zx_status_t validate_resource_mmio(ZxProcess *proc,
        zx_handle_t handle, uintptr_t base, size_t length)
{
    if (length < 1 || UINT64_MAX - base < length) {
        return ZX_ERR_INVALID_ARGS;
    }
    return validate_ranged_resource(proc, handle,
            ZX_RSRC_KIND_MMIO, base, base + length - 1);
}

/* Validates creation of an interrupt object based on a resource handle */
static inline zx_status_t validate_resource_irq(ZxProcess *proc,
        zx_handle_t handle, uint32_t irq)
{
    return validate_ranged_resource(proc, handle, ZX_RSRC_KIND_IRQ, irq, irq);
}
