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

class ZxVmar;

class ZxVmar final : public ZxObject {
public:
    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMAR; }
private:
    /* Parent, child vmars */
    ZxVmar *parent_;
    LinkedList<ZxVmar> children_;

    /* List of VMOs mapped in */
    LinkedList<ZxVmo> vmo_list_;

    /* start and end of addr space */
    uintptr_t base_;
    size_t size_;

    /* sel4 page structs */
}
