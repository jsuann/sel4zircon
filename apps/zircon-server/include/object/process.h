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
//#include "thread.h"

class ZxProcess final : public ZxObject {
public:
    ZxProcess(ZxVmar *root_vmar, seL4_Word badge_val) : handle_list_(this),
            root_vmar_{root_vmar}, badge_val_{badge_val}/*, thread_list_(this) */{
        memset(name_, 0, ZX_MAX_NAME_LEN);
        /* FIXME gen better rand val */
        handle_rand_ = get_koid();
    }

    ~ZxProcess() final {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_PROCESS; }

    void set_name(const char *name) {
        /* Silently truncate name */
        strncpy(name_, name, ZX_MAX_NAME_LEN-1);
    }

    const char *get_name() const {
        return name_;
    }

    void add_handle(Handle *h) {
        h->set_owner(this);
        handle_list_.push_back(h);
    }

    zx_handle_t get_handle_user_val(Handle *h) const {
        return (h->get_value() ^ handle_rand_);
    }

    Handle *get_handle(zx_handle_t user_val) const {
        Handle *h = base_value_to_addr(user_val ^ handle_rand_);
        return (h != NULL && h->get_owner() == this) ? h : NULL;
    }

    uint32_t get_handle_kernel_val(zx_handle_t user_val) const {
        return (user_val ^ handle_rand_);
    }

    bool has_handle(Handle *h) const {
        return (h->get_owner() == this);
    }

    void remove_handle(Handle *h) {
        handle_list_.remove(h);
        h->set_owner(NULL);
    }

    void print_handles() {
        auto print_func = [] (Handle *h) {
            dprintf(INFO, "Handle at %p:\n", h);
            dprintf(INFO, "%p %p %u %u\n", h->get_owner(), h->get_object(),
                    h->get_rights(), h->get_value());
        };
        handle_list_.for_each(print_func);
    }

    uint64_t get_badge_val() const {
        return badge_val_;
    }

private:
    /* List of Handles */
    LinkedList<Handle> handle_list_;
    /* Root vmar */
    ZxVmar *root_vmar_;
    /* Thread list */
    //LinkedList<ZxThread> thread_list_;

    /* Mask for ID-ing handle */
    uint32_t handle_rand_ = 0;

    int retcode_ = 0;

    char name_[ZX_MAX_NAME_LEN];

    uint64_t badge_val_;

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

void init_proc_table();
ZxProcess *get_proc_from_badge(uint64_t badge);

template <>
ZxProcess *allocate_object<ZxProcess>(ZxVmar *root_vmar);
template <>
void free_object<ZxProcess>(ZxProcess *obj);
