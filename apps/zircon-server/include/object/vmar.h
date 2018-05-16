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
class VmoMapping;

class ZxVmar final : public ZxObject, public VmRegion {
friend void deactivate_maybe_destroy_vmar(ZxVmar *root);
public:
    static constexpr uint32_t baseVmarFlags =
            ZX_VM_FLAG_CAN_MAP_SPECIFIC | ZX_VM_FLAG_CAN_MAP_READ |
            ZX_VM_FLAG_CAN_MAP_WRITE | ZX_VM_FLAG_CAN_MAP_EXECUTE;

    /* Root vmar constructor */
    ZxVmar() : parent_{NULL}, base_{ZX_USER_ASPACE_BASE},
            size_{ZX_USER_ASPACE_SIZE}, flags_{baseVmarFlags} {}

    /* Child vmar constructor */
    ZxVmar(ZxVmar *parent, uintptr_t base, size_t size, uint32_t flags) :
            parent_{parent}, base_{base}, size_{size}, flags_{flags & baseVmarFlags} {
        proc_ = parent->get_proc();
    }

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMAR; }

    void destroy() override;

    bool can_destroy() override {
        /* Vmar must already be destroyed by vmar_destroy */
        return (zero_handles() && !is_active_);
    }

    bool is_deactivated() const {
        return !is_active_;
    }

    uintptr_t get_base() const override { return base_; }
    size_t get_size() const override { return size_; }
    VmRegion *get_parent() const override { return parent_; }
    bool is_vmar() const override { return true; }
    bool is_vmo_mapping() const override { return false; }

    bool check_vm_region(uintptr_t child_base, size_t child_size);
    bool add_vm_region(VmRegion *child);

    zx_status_t unmap_regions(uintptr_t addr, size_t len);
    zx_status_t update_prot(uintptr_t addr, size_t len, uint32_t flags);

    /* Allocate a base offset for non-specific regions */
    uintptr_t allocate_vm_region_base(uintptr_t size, uint32_t flags);

    VmoMapping *get_vmap_from_addr(uintptr_t addr);

    void set_proc(ZxProcess *proc) {
        proc_ = proc;
    }

    ZxProcess *get_proc() const {
        return proc_;
    }

    uint32_t get_flags() const {
        return flags_;
    }

    /* Convert vmar flags into the maximal set of
       handle rights for this vmar */
    zx_rights_t get_rights() const {
        zx_rights_t rights = ZX_DEFAULT_VMAR_RIGHTS;
        if (flags_ & ZX_VM_FLAG_CAN_MAP_READ) {
            rights |= ZX_RIGHT_READ;
        }
        if (flags_ & ZX_VM_FLAG_CAN_MAP_WRITE) {
            rights |= ZX_RIGHT_WRITE;
        }
        if (flags_ & ZX_VM_FLAG_CAN_MAP_EXECUTE) {
            rights |= ZX_RIGHT_EXECUTE;
        }
        return rights;
    }

private:
    /* Owning process */
    ZxProcess *proc_;

    /* Parent vmar */
    ZxVmar *parent_;

    /* False if vmar has been destroyed */
    bool is_active_ = true;

    /* List of child vmars & mapped vmos */
    Vector<VmRegion*> children_;

    /* start and end of addr space */
    uintptr_t base_;
    size_t size_;

    /* vmar flags */
    uint32_t flags_;
};

void deactivate_maybe_destroy_vmar(ZxVmar *root);
