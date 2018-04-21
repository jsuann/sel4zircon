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

class ZxChannel final : public ZxObject {
friend zx_status_t create_channel_pair(ZxChannel *&ch1, ZxChannel *&ch2);
public:
    ZxChannel() = default;

    void destroy() override;

    zx_status_t write_msg(void* bytes, uint32_t num_bytes,
            zx_handle_t* handles, uint32_t num_handles);

    zx_status_t read_msg(void *bytes, uint32_t *num_bytes,
            Handle **handles, uint32_t *num_handles);

private:
    /* Messages in a channel simply describe the num
       handles/bytes written to their respective containers */
    struct Message : public Listable<Message> {
        uint32_t num_handles_;
        uint32_t num_bytes_;
    };

    /* List (queue) of handles */
    LinkedList<Handle> handle_list_;

    /* Mbuf for message data */
    Mbuf data_buf_;

    LinkedList<Message> msg_list_;

    ZxChannel *peer_ = NULL;
};

zx_status_t create_channel_pair(ZxChannel *&ch1, ZxChannel *&ch2);
