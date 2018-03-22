#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

class ZxVmo final : public ZxObject {
public:
    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMO; }

private:
    struct VkaObjectNode;
    struct VmoMapping;

    /* We need keep a list of backing objects (PTs, etc) */
    struct VkaObjectNode {
        vka_object_t obj;
        struct VkaObjectNode *next;
    }

    /* Struct for maintaining mapping of VMOs */
    struct VmoMapping {
        /* start address of vmo in process */
        uintptr_t start_addr;
        /* caps to page frames */
        seL4_CPtr caps[];
        /* list of backing objects */
        struct VkaObjectNode *head;
    };

    uint64_t size_;
    uint32_t num_pages_;

    /* TODO Clone info */
    /* Linkedlist of children? */

    /* Frame objects of the vmo */
    vka_object_t frames_[];

    /* Server mapping info */
    uintptr_t kaddr_;
    struct VkaObjectNode *khead_;

    /* process mapping of the vmo */
    // TODO vector?
    VmoMapping *proc_map_;
};
