#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <sel4utils/process.h>
#include <zircon/types.h>
}

#include "linkedlist.h"
#include "handle.h"
#include "object.h"

class ZxProcess final : public ZxObject {
public:
    ZxProcess(zx_koid_t koid) : ZxObject(koid), handle_list_(this) {}
    ~ZxProcess() final {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_PROCESS; }

    void add_handle(Handle *h) {
        handle_list_.push_back(h);
    }

private:
    /* List of Handles */
    LinkedList<Handle> handle_list_;
    /* TODO root vmar */
    /* TODO thread list */
    /* TODO sel4 specific stuff */
    /*
       Will probably need:
       - cspace
       - capptr to process endpoint
    */
};
