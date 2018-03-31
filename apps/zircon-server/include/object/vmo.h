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
#include "vmar.h"

class VmoMapping final : public Listable<VmoMapping>, public VmRegion {
    friend class ZxVmo;
public:
    VmoMapping(uintptr_t start_addr, ZxVmar *parent) :
            start_addr_{start_addr}, parent_{parent} {}

    uintptr_t get_base() const override { return start_addr_; }
    size_t get_size() const override { return ((ZxVmo*)get_owner())->size_; }
    VmRegion *get_parent() const override { return parent_; }
    bool is_vmar() const override { return false; }
    bool is_vmo_mapping() const override { return true; }

    bool init();
    void destroy();

private:
    /* start address of vmo in process */
    uintptr_t start_addr_;
    /* caps to page frames */
    seL4_CPtr *caps_ = NULL;
    /* ptr back to owning vmar */
    ZxVmar *parent_;
};

class ZxVmo final : public ZxObject {
public:
    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMO; }

    ZxVmo(uintptr_t kaddr, uint32_t num_pages) : kaddr_{kaddr},
            num_pages_{num_pages}, map_list_{this} {
        size_ = num_pages * (1 << seL4_PageBits);
    }

    bool init();
    void destroy();

    VmoMapping *create_mapping(uintptr_t start_addr, ZxVmar *vmar);

private:
    /* Server mapping address */
    uintptr_t kaddr_;

    uint32_t num_pages_;
    size_t size_;

    /* TODO Clone info */

    /* Frame objects of the vmo */
    vka_object_t *frames_ = NULL;

    /* process mappings of the vmo */
    LinkedList<VmoMapping> map_list_;
    uint32_t num_mappings_ = 0;
};
