#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

#include "linkedlist.h"
#include "object.h"
#include "vm_region.h"
#include "../addrspace.h"

class ZxProcess;

class ZxVmar final : public ZxObject, public VmRegion {
public:
    /* Root vmar constructor */
    ZxVmar() : parent_{NULL}, base_{ZX_USER_ASPACE_BASE},
            size_{ZX_USER_ASPACE_SIZE} {}

    /* Child vmar constructor */
    ZxVmar(ZxVmar *parent, uintptr_t base, size_t size) : parent_{parent},
            base_{base}, size_{size} {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMAR; }

    uintptr_t get_base() const override { return base_; }
    size_t get_size() const override { return size_; }
    VmRegion *get_parent() const override { return parent_; }
    bool is_vmar() const override { return true; }
    bool is_vmo_mapping() const override { return false; }

    bool check_vm_region(uintptr_t child_base, size_t child_size);
    bool add_vm_region(VmRegion *child);

    void set_proc(ZxProcess *proc) {
        proc_ = proc;
    }

    ZxProcess *get_proc() const {
        return proc_;
    }

private:
    /* Owning process */
    ZxProcess *proc_;

    /* Parent vmar */
    ZxVmar *parent_;

    /* List of child vmars & mapped vmos */
    Vector<VmRegion*> children_;

    /* start and end of addr space */
    uintptr_t base_;
    size_t size_;
};
