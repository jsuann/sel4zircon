#include "object/socket.h"

zx_status_t create_socket_pair(uint32_t flags, ZxSocket *&sock0,
        ZxSocket *&sock1)
{
    sock0 = sock1 = NULL;

    zx_signals_t starting_signals = ZX_SOCKET_WRITABLE;
    if (flags & ZX_SOCKET_HAS_ACCEPT) {
        starting_signals |= ZX_SOCKET_SHARE;
    }
    if (flags & ZX_SOCKET_HAS_CONTROL) {
        starting_signals |= ZX_SOCKET_CONTROL_WRITABLE;
    }

    sock0 = allocate_object<ZxSocket>(starting_signals, flags);
    sock1 = allocate_object<ZxSocket>(starting_signals, flags);
    if (sock0 == NULL || sock1 == NULL) {
        goto error_socket_pair;
    }

    if (flags & ZX_SOCKET_HAS_CONTROL) {
        /* Create control msgs */
        sock0->control_msg_ = allocate_object<ZxSocket::ControlMsg>();
        sock1->control_msg_ = allocate_object<ZxSocket::ControlMsg>();
        if (sock0->control_msg_ == NULL || sock1->control_msg_ == NULL) {
            goto error_socket_pair;
        }
    }

    /* Set peers */
    sock0->peer_ = sock1;
    sock1->peer_ = sock0;

    return ZX_OK;

error_socket_pair:
    if (sock0 != NULL) {
        destroy_object(sock0);
    }

    if (sock1 != NULL) {
        destroy_object(sock0);
    }

    return ZX_ERR_NO_MEMORY;
}

void ZxSocket::destroy()
{
    /* Let peer know we are being destroyed */
    ZxSocket *other = peer_;
    if (other != NULL) {
        other->peer_ = NULL;
        other->update_state(0u, ZX_SOCKET_PEER_CLOSED);
    }

    /* Clear data buf */
    data_buf_.clear();

    /* Clear datagram list */
    while (!datagrams_.empty()) {
        delete datagrams_.pop_front();
    }

    /* Delete handle in accept queue */
    if (accept_queue_ != NULL) {
        destroy_handle_maybe_object(accept_queue_);
    }

    /* Free control msg */
    if (control_msg_ != NULL) {
        delete control_msg_;
    }
}

/* called by peer */
zx_status_t ZxSocket::write(void *src, size_t len, size_t* written)
{
    if (data_buf_.is_full()) {
        return ZX_ERR_SHOULD_WAIT;
    }

    size_t old_size = data_buf_.get_size();
    zx_status_t err;
    size_t actual_len;

    /* Do write depending on socket type */
    if (flags_ & ZX_SOCKET_DATAGRAM) {
        Datagram *dg = allocate_object<Datagram>(len);
        if (dg == NULL) {
            return ZX_ERR_NO_MEMORY;
        }
        err = data_buf_.write((uint8_t *)src, len, true);
        if (err) {
            delete dg;
            return err;
        }
        actual_len = len;
        datagrams_.push_back(dg);
    } else {
        err = data_buf_.write((uint8_t *)src, len, false);
        if (err) {
            return err;
        }
        actual_len = data_buf_.get_size() - old_size;
    }

    /* Update states */
    if (actual_len > 0 && old_size == 0) {
        update_state(0u, ZX_SOCKET_READABLE);
    }

    if (peer_ != NULL && data_buf_.is_full()) {
        peer_->update_state(ZX_SOCKET_WRITABLE, 0u);
    }

    *written = actual_len;
    return ZX_OK;
}

zx_status_t ZxSocket::read(void *dest, size_t len, size_t *nread)
{
    if (dest == NULL && len == 0) {
        /* Special case: query for bytes remaining */
        *nread = data_buf_.get_size();
        return ZX_OK;
    }

    if (data_buf_.is_empty()) {
        if (peer_ == NULL) {
            return ZX_ERR_PEER_CLOSED;
        }
        if (read_disabled_) {
            return ZX_ERR_BAD_STATE;
        }
        return ZX_ERR_SHOULD_WAIT;
    }

    bool was_full = data_buf_.is_full();

    if (flags_ & ZX_SOCKET_DATAGRAM) {
        Datagram *dg = datagrams_.pop_front();
        if (len > dg->len_) {
            len = dg->len_;
        }
        data_buf_.read((uint8_t *)dest, len);
        /* discard leftover bytes in datagram */
        data_buf_.discard(dg->len_ - len);
        delete dg;
        *nread = len;
    } else {
        size_t old_size = data_buf_.get_size();
        data_buf_.read((uint8_t *)dest, len);
        *nread = old_size - data_buf_.get_size();
    }

    if (data_buf_.is_empty()) {
        uint32_t set_mask = 0u;
        if (read_disabled_) {
            set_mask |= ZX_SOCKET_READ_DISABLED;
        }
        update_state(ZX_SOCKET_READABLE, set_mask);
    }

    if (peer_ != NULL && was_full && *nread > 0) {
        peer_->update_state(0u, ZX_SOCKET_WRITABLE);
    }

    return ZX_OK;
}

/* called by peer */
zx_status_t ZxSocket::write_control(void *src, size_t len)
{
    if ((flags_ & ZX_SOCKET_HAS_CONTROL) == 0) {
        return ZX_ERR_BAD_STATE;
    }

    if (len == 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    if (len > ControlMsg::kSize) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    if (control_msg_len_ != 0) {
        return ZX_ERR_SHOULD_WAIT;
    }

    memcpy(&control_msg_->msg_, src, len);
    control_msg_len_ = len;

    update_state(0u, ZX_SOCKET_CONTROL_READABLE);
    if (peer_ != NULL) {
        peer_->update_state(ZX_SOCKET_CONTROL_WRITABLE, 0u);
    }

    return ZX_OK;
}

zx_status_t ZxSocket::read_control(void *dest, size_t len, size_t* nread)
{
    if ((flags_ & ZX_SOCKET_HAS_CONTROL) == 0) {
        return ZX_ERR_BAD_STATE;
    }

    if (control_msg_len_ == 0) {
        return ZX_ERR_SHOULD_WAIT;
    }

    size_t copy_len = (len < control_msg_len_) ? len : control_msg_len_;
    memcpy(dest, &control_msg_->msg_, copy_len);

    control_msg_len_ = 0;
    update_state(ZX_SOCKET_CONTROL_READABLE, 0u);
    if (peer_ != NULL) {
        peer_->update_state(0u, ZX_SOCKET_CONTROL_WRITABLE);
    }

    *nread = copy_len;
    return ZX_OK;
}

/* called by peer */
zx_status_t ZxSocket::share(Handle *h)
{
    if (!(flags_ & ZX_SOCKET_HAS_ACCEPT)) {
        return ZX_ERR_NOT_SUPPORTED;
    }

    if (accept_queue_ != NULL) {
        return ZX_ERR_SHOULD_WAIT;
    }

    accept_queue_ = h;

    update_state(0, ZX_SOCKET_ACCEPT);
    if (peer_ != NULL) {
        peer_->update_state(ZX_SOCKET_SHARE, 0);
    }

    return ZX_OK;
}

zx_status_t ZxSocket::accept(Handle **h)
{
    if (!(flags_ & ZX_SOCKET_HAS_ACCEPT)) {
        return ZX_ERR_NOT_SUPPORTED;
    }

    if (accept_queue_ == NULL) {
        return ZX_ERR_SHOULD_WAIT;
    }

    *h = accept_queue_;
    accept_queue_ = NULL;

    update_state(ZX_SOCKET_ACCEPT, 0);
    if (peer_ != NULL) {
        peer_->update_state(0, ZX_SOCKET_SHARE);
    }

    return ZX_OK;
}

zx_status_t ZxSocket::shutdown(uint32_t how)
{
    const bool shutdown_read = how & ZX_SOCKET_SHUTDOWN_READ;
    const bool shutdown_write = how & ZX_SOCKET_SHUTDOWN_WRITE;

    /* If we're already shut down in the requested way, return immediately. */
    zx_signals_t signals = get_signals();
    const uint32_t want_signals =
        (shutdown_read ? ZX_SOCKET_READ_DISABLED : 0) |
        (shutdown_write ? ZX_SOCKET_WRITE_DISABLED : 0);
    const uint32_t have_signals = signals & (ZX_SOCKET_READ_DISABLED | ZX_SOCKET_WRITE_DISABLED);
    if (want_signals == have_signals) {
        return ZX_OK;
    }

    zx_signals_t clear_mask = 0u;
    zx_signals_t set_mask = 0u;
    if (shutdown_read) {
        /* Disable reading once remaining data has been read */
        read_disabled_ = true;
        if (data_buf_.is_empty()) {
            set_mask |= ZX_SOCKET_READ_DISABLED;
        }
    }
    if (shutdown_write) {
        clear_mask |= ZX_SOCKET_WRITABLE;
        set_mask |= ZX_SOCKET_WRITE_DISABLED;
    }
    update_state(clear_mask, set_mask);

    /* Our peer already be closed - if so, we've already updated our own bits so we are
       done. If the peer is done, we need to notify them of the state change. */
    if (peer_ != NULL) {
        return peer_->shutdown_by_peer(how);
    } else {
        return ZX_OK;
    }
}

zx_status_t ZxSocket::shutdown_by_peer(uint32_t how)
{
    const bool shutdown_read = how & ZX_SOCKET_SHUTDOWN_READ;
    const bool shutdown_write = how & ZX_SOCKET_SHUTDOWN_WRITE;

    zx_signals_t clear_mask = 0u;
    zx_signals_t set_mask = 0u;
    if (shutdown_read) {
        /* If the other end shut down reading, we can't write any more. */
        clear_mask |= ZX_SOCKET_WRITABLE;
        set_mask |= ZX_SOCKET_WRITE_DISABLED;
    }
    if (shutdown_write) {
        /* If the other end shut down writing, disable reading once
           remaining data has been read. */
        read_disabled_ = true;
        if (data_buf_.is_empty()) {
            set_mask |= ZX_SOCKET_READ_DISABLED;
        }
    }

    update_state(clear_mask, set_mask);
    return ZX_OK;
}
