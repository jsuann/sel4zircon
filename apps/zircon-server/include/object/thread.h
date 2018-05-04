#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
#include <sel4zircon/cspace.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <vspace/vspace.h>
#include <sel4utils/vspace.h>
}

#include "listable.h"
#include "object.h"
#include "waiter.h"
#include "../utils/clock.h"
#include "../addrspace.h"

class ZxThread final : public ZxObject, public Listable<ZxThread> {
public:
    ZxThread(uint32_t thread_index) : thread_index_{thread_index} {}

    ~ZxThread() final {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_THREAD; }

    void set_name(const char *name) {
        /* Silently truncate name */
        strncpy(name_, name, ZX_MAX_NAME_LEN-1);
    }

    /* Init & destroy seL4 data */
    bool init();
    void destroy() override;

    /* TODO can destroy() only when thread is dead */

    int configure_tcb(seL4_CNode pd, uintptr_t ipc_buffer_addr);

    int start_execution(uintptr_t entry, uintptr_t stack,
            uintptr_t arg1, uintptr_t arg2);

    uint32_t get_thread_index() const {
        return thread_index_;
    }

    uint32_t get_ipc_index() const {
        return ipc_index_;
    }

    void set_ipc_buffer(vka_object_t ipc_buf, uintptr_t ipc_index) {
        ipc_buffer_frame_ = ipc_buf;
        ipc_index_ = ipc_index;
    }

    void destroy_ipc_buffer();

    void wait(timer_callback_func cb, void *data,
            uint64_t expire_time, uint32_t flags);

    /* TODO stop wait when thread is killed */

    void resume_from_wait(zx_status_t ret) {
        /* Send to reply cap */
        seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, ret);
        seL4_Send(reply_cap_, tag);
    }

    void cancel_waiters();

    bool obj_wait_one(Handle *h, zx_signals_t,
            zx_time_t deadline, zx_signals_t *observed);
    bool obj_wait_many(Handle *handles, uint32_t count,
            zx_time_t deadline, zx_wait_item_t* items);
    void signal_observed(StateWaiter *sw) {}; //XXX

    uint64_t runtime_ns() {
        /* Should be amount of actual cpu time, but just
           get time from creation for now */
        return get_system_time() - start_time_;
    }

private:
    /* Use thread index to get badge values */
    uint32_t thread_index_;

    /* Index used to calculate location of IPC buffer in addrspace */
    uint32_t ipc_index_;

    /* Starting register values */
    /*
    uintptr_t user_entry_ = 0;
    uintptr_t user_sp_ = 0;
    uintptr_t user_arg1_ = 0;
    uintptr_t user_arg2_ = 0;
    */

    uint64_t start_time_ = 0;

    /* TimerNode for waits */
    TimerNode timer_;

    /* Non-null if we are waiting on 1+ objects */
    Waiter *waiting_on_ = NULL;
    void *wait_data_ = NULL;
    uint32_t num_waiting_on_ = 0;
    uint32_t rem_waiting_on_ = 0;

    /* List of waiters on this */

    /* State */

    /* Exception port */

    char name_[ZX_MAX_NAME_LEN] = {0};

    /* cspace */
    vka_object_t cspace_ = {0};

    /* IPC buffer frame */
    vka_object_t ipc_buffer_frame_ = {0};

    /* TCB */
    vka_object_t tcb_ = {0};

    /* TODO RT scheduler */
    /* vka_object_t sched_context_ = {0}; */

    /* If waiting, slot for reply cap */
    seL4_CPtr reply_cap_ = 0;
};

void init_thread_table(vspace_t *vspace);

ZxThread *get_thread_from_badge(uint64_t badge);

template <>
ZxThread *allocate_object<ZxThread>();
template <>
void free_object<ZxThread>(ZxThread *obj);

/* timer callback funcs */
void nanosleep_cb(void *data);
