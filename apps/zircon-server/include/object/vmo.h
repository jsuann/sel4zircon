#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

struct VmoMapping {
    uintptr_t start_addr;
    /* caps to page frames */
    seL4_CPtr caps[];
};

class ZxVmo final : public ZxObject {
public:
    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMO; }
private:
    uint64_t size_;
    uint32_t num_pages_;

    /* TODO Clone info */
    ZxVmo *parent_;
    /* Linkedlist of children? */

    /* server mapping of the vmo */
    VmoMapping kmap_;

    /* process mapping of the vmo */
    // TODO vector?
    VmoMapping *proc_map_;
};
