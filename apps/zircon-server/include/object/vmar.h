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
#include "../addrspace.h"

class ZxVmar;

class ZxVmar final : public ZxObject, public Listable<ZxVmar> {
public:
    /* Root vmar constructor */
    ZxVmar() : children_{this}, base_{ZX_USER_ASPACE_BASE},
            size_{ZX_USER_ASPACE_SIZE} {}

    /* Child vmar constructor */
    ZxVmar(uintptr_t base, ssize_t size) : children_{this},
            base_{base}, size_{size} {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMAR; }
private:
    /* Parent, child vmars */
    LinkedList<ZxVmar> children_;

    /* List of VMOs mapped in */
    //LinkedList<ZxVmo> vmo_list_;
    /* List of vmo cap mappings */

    /* start and end of addr space */
    uintptr_t base_;
    ssize_t size_;

    /* sel4 page structs */
};
