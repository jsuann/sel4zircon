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

constexpr uintptr_t root_base = (1024 * 1024);
constexpr ssize_t root_size = ((1ull << 47) - root_base);

class ZxVmar;

class ZxVmar final : public ZxObject, public Listable<ZxVmar> {
public:
    /* Root vmar constructor */
    ZxVmar() : parent_{NULL}, children_{this}, base_{root_base},
            size_{root_size} {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMAR; }
private:
    /* Parent, child vmars */
    ZxVmar *parent_;
    LinkedList<ZxVmar> children_;

    /* List of VMOs mapped in */
    //LinkedList<ZxVmo> vmo_list_;
    /* List of vmo cap mappings */

    /* start and end of addr space */
    uintptr_t base_;
    ssize_t size_;

    /* sel4 page structs */
};
