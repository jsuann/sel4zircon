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

class ZxJob;

class ZxProcess final : public ZxObjectWaitable, public Listable<ZxProcess> {
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
    void destroy() override;

    bool can_destroy() override {
        /* Can't destroy while running or killing off threads */
        if (state_ == State::RUNNING || state_ == State::KILLING) {
            return false;
        }

        /* Otherwise process can be destroyed once all threads removed */
        return (zero_handles() && thread_list_.empty());
    }

    void set_name(const char *name) {
        /* Silently truncate name */
        strncpy(name_, name, ZX_MAX_NAME_LEN - 1);
    }

    const char *get_name() const {
        return name_;
    }

    ZxVmar *get_root_vmar() const {
        return root_vmar_;
    }

    uint32_t get_proc_index() const {
        return proc_index_;
    }

    enum class State {
        INITIAL,    /* Proc created, but no thread started yet */
        RUNNING,    /* First thread started */
        KILLING,    /* Killing off threads */
        DEAD,       /* All threads dead */
    };

    bool is_running() const { return (state_ == State::RUNNING); }
    bool is_killing() const { return (state_ == State::KILLING); }
    bool is_dead() const { return (state_ == State::DEAD); }

    void thread_started() {
        if (state_ == State::INITIAL) {
            state_ = State::RUNNING;
        }

        ++alive_count_;
    }

    bool thread_exited() {
        --alive_count_;
        /* Return true if process should also exit */
        return (alive_count_ == 0);
    }

    void set_retcode(int retcode) { retcode_ = retcode; }
    void kill();

    /* Handle funcs */
    void add_handle(Handle *h) {
        handle_list_.push_back(h);
    }

    zx_handle_t get_handle_user_val(Handle *h) const {
        return (((h->get_value() << 1) | 0x1) ^ handle_rand_);
    }

    Handle *get_handle(zx_handle_t user_val) const {
        Handle *h = base_value_to_addr((user_val ^ handle_rand_) >> 1);
        return (h != NULL && h->get_owner() == this) ? h : NULL;
    }

    bool has_handle(Handle *h) const {
        return (h->get_owner() == this);
    }

    void remove_handle(Handle *h) {
        handle_list_.remove(h);
    }

    void print_handles() {
        auto print_func = [](Handle * h) {
            dprintf(INFO, "Handle at %p:\n", h);
            dprintf(INFO, "proc: %p\tobj: %p\trights: %u\tvalue: %u\n",
                    h->get_owner(), h->get_object(), h->get_rights(), h->get_value());
        };
        handle_list_.for_each(print_func);
    }

    /* Thread funcs */
    bool add_thread(ZxThread *thrd);
    void remove_thread(ZxThread *thrd);

    /* Addrspace funcs */
    int map_page_in_vspace(seL4_CPtr frame_cap, void *vaddr,
            seL4_CapRights_t rights, int cacheable);
    void remap_page_in_vspace(seL4_CPtr frame_cap,
            seL4_CapRights_t rights, int cacheable);

    zx_status_t uvaddr_to_kvaddr(uintptr_t uvaddr, size_t len, void *&kvaddr);

    /* Wrapper for above when getting specific types */
    template <typename T>
    zx_status_t get_kvaddr(uintptr_t uvaddr, T *&t) {
        return uvaddr_to_kvaddr(uvaddr, sizeof(T), (void *&)t);
    }

    /* Object funcs */
    template <typename T>
    zx_status_t get_object_with_rights(zx_handle_t handle_val,
            zx_rights_t rights, T *&obj);

    template <typename T>
    zx_status_t get_object(zx_handle_t handle_val, T *&obj) {
        return get_object_with_rights(handle_val, 0, obj);
    }

    /* Creates handle, adds to proc, returns uval */
    template <typename T>
    zx_handle_t create_handle_get_uval(T *obj) {
        Handle *h = create_handle_default_rights<T>(obj);

        if (h == NULL) {
            return ZX_HANDLE_INVALID;
        }

        add_handle(h);
        return get_handle_user_val(h);
    }

private:
    /* List of Handles */
    LinkedList<Handle> handle_list_;
    /* Root vmar */
    ZxVmar *root_vmar_;
    /* Thread list */
    LinkedList<ZxThread> thread_list_;
    int alive_count_ = 0;

    /* IPC buffer index allocator */
    BitAlloc ipc_alloc_;

    /* Mask for ID-ing handle */
    uint32_t handle_rand_ = 0;

    int retcode_ = 0;

    char name_[ZX_MAX_NAME_LEN] = {0};

    uint32_t proc_index_;

    /* State */
    State state_ = State::INITIAL;

    /* Exception port */

    /* vspace */
    vka_object_t pd_ = {0};
    seL4_CPtr asid_pool_ = 0;

    /* Used to store PT objects */
    VkaObjectNode *pt_list_ = NULL;
};

void init_proc_table(vspace_t *vspace);
void init_asid_pool(vka_t *vka);
ZxProcess *get_proc_from_badge(uint64_t badge);

template <>
ZxProcess *allocate_object<ZxProcess>(ZxVmar *root_vmar);
template <>
void free_object<ZxProcess>(ZxProcess *obj);
