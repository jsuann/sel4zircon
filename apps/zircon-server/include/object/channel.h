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
#include "handle.h"
#include "mbuf.h"

class ZxProcess;

class ZxChannel final : public ZxObjectWaitable {
    friend zx_status_t create_channel_pair(ZxChannel *&ch0, ZxChannel *&ch1);
public:
    ZxChannel() : ZxObjectWaitable(ZX_CHANNEL_WRITABLE),
        handle_list_{this}, msg_list_{this} {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_CHANNEL; }

    void destroy() override;

    zx_status_t write_msg(void *bytes, uint32_t num_bytes,
            Handle **handles, uint32_t num_handles);

    zx_status_t read_msg(void *bytes, uint32_t *num_bytes,
            Handle **handles, uint32_t *num_handles, bool may_discard);

    ZxChannel *get_peer() const {
        return peer_;
    }

private:
    /* Messages in a channel simply describe the num
       handles/bytes written to their respective containers */
    struct Message : public Listable<Message> {
        Message(uint32_t num_handles, uint32_t num_bytes) :
            num_handles_{num_handles}, num_bytes_{num_bytes} {}
        uint32_t num_handles_;
        uint32_t num_bytes_;
    };

    /* List (queue) of handles */
    LinkedList<Handle> handle_list_;

    /* Mbuf for message data */
    MBuf data_buf_;

    LinkedList<Message> msg_list_;

    /* TODO threads waiting from channel call */

    ZxChannel *peer_ = NULL;
};

zx_status_t create_channel_pair(ZxChannel *&ch0, ZxChannel *&ch1);
