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

class ZxThread final : public ZxObject, public Listable<ZxThread> {
public:
    ZxThread(uint32_t proc_index, uint32_t thread_index) :
            proc_index_{proc_index}, thread_index_{thread_index} {}

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

    int copy_cap_to_thread(cspacepath_t *src, seL4_CPtr slot);
    int configure_tcb(seL4_CNode pd);

    int write_registers(seL4_UserContext *context, int resume) {
        return seL4_TCB_WriteRegisters(tcb_.cptr, resume, 0,
                sizeof(*context) / sizeof(seL4_Word), context);
    }

    uint32_t get_thread_index() const {
        return thread_index_;
    }

    void set_ipc_buffer(vka_object_t ipc_buf, uintptr_t ipc_buf_addr) {
        ipc_buffer_frame_ = ipc_buf;
        ipc_buffer_addr_ = ipc_buf_addr;
    }

private:
    /* We need index of proc & thread to get badge value */
    uint32_t proc_index_;
    uint32_t thread_index_;

    /* Starting register values */
    uintptr_t user_entry_ = 0;
    uintptr_t user_sp_ = 0;
    uintptr_t user_arg1_ = 0;
    uintptr_t user_arg2_ = 0;

    /* State */
    /* Exception port */

    char name_[ZX_MAX_NAME_LEN] = {0};

    /* cspace */
    vka_object_t cspace_ = {0};

    /* IPC buffer frame */
    vka_object_t ipc_buffer_frame_ = {0};
    uintptr_t ipc_buffer_addr_ = 0;

    /* TCB */
    vka_object_t tcb_ = {0};

    /* TODO RT scheduler */
    vka_object_t sched_context_ = {0};

    /* If waiting, slot for reply cap */
    seL4_CPtr reply_cap_ = 0;   // vka object for RT kernel
};
