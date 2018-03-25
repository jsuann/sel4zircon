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
    uintptr_t get_start_address() const override { return start_address_; }
    bool is_vmar() const override { return false; }
    bool is_vmo_mapping() const override { return true; }
private:
    /* start address of vmo in process */
    uintptr_t start_addr_;
    /* caps to page frames */
    seL4_CPtr caps_[];
    /* ptr back to owning vmar */
    ZxVmar *vmar_;
};


class ZxVmo final : public ZxObject {
public:
    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMO; }

private:
    uint64_t size_;
    uint32_t num_pages_;

    /* TODO Clone info */
    /* Linkedlist of children? */

    /* Frame objects of the vmo */
    vka_object_t frames_[];

    /* Server mapping info */
    uintptr_t kaddr_;

    /* process mapping of the vmo */
    LinkedList<VmoMapping> proc_map_;
    size_t num_mappings_ = 0;
};
