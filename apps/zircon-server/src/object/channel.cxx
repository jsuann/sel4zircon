#include "object/channel.h"
#include "object/process.h"

zx_status_t create_channel_pair(ZxChannel *&ch0, ZxChannel *&ch1)
{
    ch0 = ch1 = NULL;

    ch0 = allocate_object<ZxChannel>();

    if (ch0 == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    ch1 = allocate_object<ZxChannel>();

    if (ch1 == NULL) {
        free_object(ch0);
        ch0 = NULL;
        return ZX_ERR_NO_MEMORY;
    }

    /* Set peers */
    ch0->peer_ = ch1;
    ch1->peer_ = ch0;

    return ZX_OK;
}

void ZxChannel::destroy()
{
    /* Let peer know we are being destroyed */
    ZxChannel *other = peer_;

    if (other != NULL) {
        other->peer_ = NULL;
        other->update_state(0u, ZX_CHANNEL_PEER_CLOSED);
    }

    /* Destroy handles */
    while (!handle_list_.empty()) {
        Handle *h = handle_list_.pop_front();
        destroy_handle_maybe_object(h);
    }

    /* Free all messages */
    while (!msg_list_.empty()) {
        Message *msg = msg_list_.pop_front();
        delete msg;
    }

    /* Clear data buf */
    data_buf_.clear();
}

/* Called by peer channel */
zx_status_t ZxChannel::write_msg(void *bytes, uint32_t num_bytes,
        Handle **handles, uint32_t num_handles)
{
    /* TODO check for waiters. If a msg has a matching txid, write
       directly to the waiter. */

    /* Allocate a new message packet */
    Message *msg = allocate_object<Message>(num_handles, num_bytes);

    if (msg == NULL) {
        dprintf(CRITICAL, "No memory for msg packet!\n");
        return ZX_ERR_NO_MEMORY;
    }

    /* Write to data buf */
    int err = data_buf_.write((uint8_t *)bytes, num_bytes, true);

    if (err != ZX_OK) {
        delete msg;
        return err;
    }

    /* Bytes successfully written, add handles & msg */
    msg_list_.push_back(msg);

    for (size_t i = 0; i < num_handles; ++i) {
        handle_list_.push_back(handles[i]);
    }

    update_state(0u, ZX_CHANNEL_READABLE);

    return ZX_OK;
}

zx_status_t ZxChannel::read_msg(void *bytes, uint32_t *num_bytes,
        Handle **handles, uint32_t *num_handles, bool may_discard)
{
    if (msg_list_.empty()) {
        return (peer_ != NULL) ? ZX_ERR_SHOULD_WAIT : ZX_ERR_PEER_CLOSED;
    }

    uint32_t max_bytes = *num_bytes;
    uint32_t max_handles = *num_handles;

    /* Get message at front of queue */
    Message *msg = msg_list_.front();
    *num_bytes = msg->num_bytes_;
    *num_handles = msg->num_handles_;

    /* Check the supplied buffers are large enough */
    zx_status_t rv = ZX_OK;

    if (*num_handles > max_handles || *num_bytes > max_bytes) {
        if (!may_discard) {
            return ZX_ERR_BUFFER_TOO_SMALL;
        }

        /* Discard msg: destroy handles, discard bytes */
        data_buf_.discard(msg->num_bytes_);

        for (size_t i = 0; i < msg->num_handles_; ++i) {
            destroy_handle_maybe_object(handle_list_.pop_front());
        }

        rv = ZX_ERR_BUFFER_TOO_SMALL;
    } else {
        /* Read msg as normal */
        data_buf_.read((uint8_t *)bytes, msg->num_bytes_);

        for (size_t i = 0; i < msg->num_handles_; ++i) {
            handles[i] = handle_list_.pop_front();
        }
    }

    /* Free message packet */
    msg_list_.pop_front();
    delete msg;

    if (msg_list_.empty()) {
        update_state(ZX_CHANNEL_READABLE, 0u);
    }

    return rv;
}
