#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <vspace/vspace.h>
#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>
#include <zircon/types.h>
}

#include "linkedlist.h"
#include "handle.h"
#include "object.h"
#include "vmar.h"
#include "thread.h"
#include "../zxcpp/bitalloc.h"
#include "vkaobjectnode.h"
#include "../utils/rng.h"

class ZxProcess final : public ZxObject, public Listable<ZxProcess> {
public:
    ZxProcess(ZxVmar *root_vmar, uint32_t proc_index) : handle_list_(this),
            root_vmar_{root_vmar}, thread_list_(this), proc_index_{proc_index} {
        handle_rand_ = get_handle_rand();
        root_vmar_->set_proc(this);
    }

    ~ZxProcess() final {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_PROCESS; }

    /* Init & destroy seL4 data */
    bool init();
    void destroy();

    /* TODO: can_destroy should check owning job */
    /* TODO: entry into dead state once all threads exited */

    void set_name(const char *name) {
        /* Silently truncate name */
        strncpy(name_, name, ZX_MAX_NAME_LEN-1);
    }

    const char *get_name() const {
        return name_;
    }

    void add_handle(Handle *h) {
        handle_list_.push_back(h);
    }

    ZxVmar *get_root_vmar() const {
        return root_vmar_;
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
    }

    void print_handles() {
        auto print_func = [] (Handle *h) {
            dprintf(INFO, "Handle at %p:\n", h);
            dprintf(INFO, "proc: %p\tobj: %p\trights: %u\tvalue: %u\n",
                    h->get_owner(), h->get_object(), h->get_rights(), h->get_value());
        };
        handle_list_.for_each(print_func);
    }

    uint32_t get_proc_index() const {
        return proc_index_;
    }

    bool alloc_thread_index(uint32_t &index) {
        return thrd_alloc_.alloc(index);
    }

    bool add_thread(ZxThread *thrd);
    int map_page_in_vspace(seL4_CPtr frame_cap, void *vaddr,
            seL4_CapRights_t rights, int cacheable);

    zx_status_t uvaddr_to_kvaddr(uintptr_t uvaddr, size_t len, void *&kvaddr);

    /* Wrapper for above when getting specific types */
    template <typename T>
    zx_status_t get_kvaddr(uintptr_t uvaddr, T *&t) {
        return uvaddr_to_kvaddr(uvaddr, sizeof(T), (void *&)t);
    }

    template <typename T>
    zx_status_t get_object_with_rights(zx_handle_t handle_val,
            zx_rights_t rights, T *&obj);

    template <typename T>
    zx_status_t get_object(zx_handle_t handle_val, T *&obj) {
        return get_object_with_rights(handle_val, 0, obj);
    }

private:
    /* List of Handles */
    LinkedList<Handle> handle_list_;
    /* Root vmar */
    ZxVmar *root_vmar_;
    /* Thread list */
    LinkedList<ZxThread> thread_list_;

    /* Thread index allocator */
    BitAlloc thrd_alloc_;

    /* Mask for ID-ing handle */
    uint32_t handle_rand_ = 0;

    int retcode_ = 0;

    char name_[ZX_MAX_NAME_LEN] = {0};

    uint32_t proc_index_;

    /* Owning Job */
    /* State */
    /* Exception port */

    /* vspace */
    vka_object_t pd_ = {0};
    seL4_CPtr asid_pool_ = 0;

    /* Used to store PT objects */
    VkaObjectNode *pt_list_ = NULL;
};

void init_proc_table(vspace_t *vspace);
ZxProcess *get_proc_from_badge(uint64_t badge);

template <>
ZxProcess *allocate_object<ZxProcess>(ZxVmar *root_vmar);
template <>
void free_object<ZxProcess>(ZxProcess *obj);
