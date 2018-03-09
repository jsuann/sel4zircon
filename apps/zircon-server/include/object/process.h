#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <sel4utils/process.h>
#include <zircon/types.h>
}

#include "linkedlist.h"
#include "handle.h"
#include "object.h"
#include "vmar.h"
#include "thread.h"

class ZxProcess final : public ZxObject {
public:
    ZxProcess(zx_koid_t koid) : ZxObject(koid), handle_list_(this), root_vmar{NULL},
            thread_list_{NULL} {
        memset(name_, 0, ZX_MAX_NAME_LEN);
    }

    ~ZxProcess() final {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_PROCESS; }

    void set_name(const char *name) {
        /* Silently truncate name */
        strncpy(name_, name, ZX_MAX_NAME_LEN-1);
    }

    char *get_name() {
        return name_;
    }

    void add_handle(Handle *h) {
        handle_list_.push_back(h);
    }

private:
    /* List of Handles */
    LinkedList<Handle> handle_list_;
    /* Root vmar */
    ZxVmar *root_vmar_;
    /* Thread list */
    LinkedList<ZxThread> thread_list_;

    /* Mask for ID-ing handle */
    uint32_t handle_rand_ = 0;

    int retcode_ = 0;

    char[ZX_MAX_NAME_LEN] name_;

    /* Owning Job */
    /* State */
    /* Exception port */

    /* TODO sel4 specific stuff */
    /*
       Will probably need:
       - cspace
       - capptr to process endpoint
       - badge value?
    */
};
