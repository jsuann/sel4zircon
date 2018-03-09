#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

class VmoMapping {

};

class ZxVmo final : public ZxObject {
public:
    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMO; }
private:
    /* VMOs can be big up to a limit (i.e. mappable in the server addr space) */
    uint64_t size_;
    uint32_t num_pages_;

    /* TODO Clone info */
    ZxVmo *parent_;
    /* Linkedlist of children? */

    /* seL4 structs */
    VmoMapping *kmapping;
};
