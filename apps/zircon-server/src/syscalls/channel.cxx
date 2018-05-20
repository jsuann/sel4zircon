#include <autoconf.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"

namespace SysChannel {

/* Helper for transferring handles from process to channel */
zx_status_t take_handles_from_proc(ZxProcess *proc, ZxChannel *channel,
        uint32_t num, zx_handle_t *in, Handle **out);

} /* namespace SysChannel */

uint64_t sys_channel_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 3);
    uint32_t options = seL4_GetMR(0);
    uintptr_t user_out0 = seL4_GetMR(1);
    uintptr_t user_out1 = seL4_GetMR(2);

    if (options != 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    zx_handle_t *out0, *out1;
    err = proc->get_kvaddr(user_out0, out0);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_out1, out1);
    SYS_RET_IF_ERR(err);

    /* Create channel pair */
    ZxChannel *ch0, *ch1;
    err = create_channel_pair(ch0, ch1);
    SYS_RET_IF_ERR(err);

    /* Create handles to channels */
    Handle *h0, *h1;
    h0 = create_handle_default_rights(ch0);
    h1 = create_handle_default_rights(ch1);
    if (h0 == NULL || h1 == NULL) {
        if (h0 != NULL) {
            ch0->destroy_handle(h0);
        }
        destroy_object(ch0);
        destroy_object(ch1);
        return ZX_ERR_NO_MEMORY;
    }

    /* Add handles to proc, set out vals */
    proc->add_handle(h0);
    proc->add_handle(h1);
    *out0 = proc->get_handle_user_val(h0);
    *out1 = proc->get_handle_user_val(h1);
    return ZX_OK;
}

uint64_t sys_channel_write(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysChannel;

    SYS_CHECK_NUM_ARGS(tag, 6);
    zx_handle_t channel_handle = seL4_GetMR(0);
    uint32_t options = seL4_GetMR(1);
    uintptr_t user_bytes = seL4_GetMR(2);
    uint32_t num_bytes = seL4_GetMR(3);
    uintptr_t user_handles = seL4_GetMR(4);
    uint32_t num_handles = seL4_GetMR(5);

    if (options != 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    if (num_bytes > ZX_CHANNEL_MAX_MSG_BYTES ||
            num_handles > ZX_CHANNEL_MAX_MSG_HANDLES) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    void* bytes;
    zx_handle_t* handles;
    if (num_bytes > 0) {
        err = proc->uvaddr_to_kvaddr(user_bytes, num_bytes, bytes);
        SYS_RET_IF_ERR(err);
    } else if (user_bytes != 0) {
        return ZX_ERR_INVALID_ARGS;
    }
    if (num_handles > 0) {
        err = proc->uvaddr_to_kvaddr(user_handles,
                sizeof(zx_handle_t) * num_handles, (void *&)handles);
        SYS_RET_IF_ERR(err);
    } else if (user_handles != 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Get channel */
    ZxChannel *channel;
    err = proc->get_object_with_rights(channel_handle, ZX_RIGHT_WRITE, channel);
    SYS_RET_IF_ERR(err);

    /* We write to the peer, so make sure it is still alive */
    if (channel->get_peer() == NULL) {
        return ZX_ERR_PEER_CLOSED;
    }

    /* Take out handles from proc so they can be added to the channel */
    Handle *h[ZX_CHANNEL_MAX_MSG_HANDLES];
    if (num_handles > 0) {
        err = take_handles_from_proc(proc, channel, num_handles, handles, &h[0]);
        SYS_RET_IF_ERR(err);
    }

    /* Write to the peer of the channel */
    err = channel->get_peer()->write_msg(bytes, num_bytes, &h[0], num_handles);
    if (err != ZX_OK) {
        /* Return handles to proc. */
        for (size_t i = 0; i < num_handles; ++i) {
            proc->add_handle(h[i]);
        }
        return err;
    }

    return ZX_OK;
}

/* Common handler for the two type of channel read */
template <typename T>
uint64_t channel_read_common(seL4_MessageInfo_t tag, uint64_t badge,
        Handle **handles, T *&handle_info, uint32_t *num)
{
    /* Disgusting arg count */
    SYS_CHECK_NUM_ARGS(tag, 8);
    zx_handle_t channel_handle = seL4_GetMR(0);
    uint32_t options = seL4_GetMR(1);
    uintptr_t user_bytes = seL4_GetMR(2);
    uintptr_t user_handles = seL4_GetMR(3);
    uint32_t num_bytes = seL4_GetMR(4);
    uint32_t num_handles = seL4_GetMR(5);
    uintptr_t user_actual_bytes = seL4_GetMR(6);
    uintptr_t user_actual_handles = seL4_GetMR(7);

    if (options & ~ZX_CHANNEL_READ_MAY_DISCARD) {
        return ZX_ERR_NOT_SUPPORTED;
    }

    if (num_bytes > ZX_CHANNEL_MAX_MSG_BYTES ||
            num_handles > ZX_CHANNEL_MAX_MSG_HANDLES) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    /* Get the ptr translations. If num to read is zero, ensure ptrs are NULL */
    void* bytes;
    if (num_bytes > 0) {
        err = proc->uvaddr_to_kvaddr(user_bytes, num_bytes, bytes);
        SYS_RET_IF_ERR(err);
    } else if (user_bytes != 0) {
        return ZX_ERR_INVALID_ARGS;
    }
    if (num_handles > 0) {
        err = proc->uvaddr_to_kvaddr(user_handles,
                sizeof(T) * num_handles, (void *&)handle_info);
        SYS_RET_IF_ERR(err);
    } else if (user_handles != 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Actual bytes/handles ptrs can also be null */
    uint32_t *actual_bytes, *actual_handles;
    actual_bytes = actual_handles = NULL;
    if (user_actual_bytes != 0) {
        err = proc->get_kvaddr(user_actual_bytes, actual_bytes);
        SYS_RET_IF_ERR(err);
    }
    if (user_actual_handles != 0) {
        err = proc->get_kvaddr(user_actual_handles, actual_handles);
        SYS_RET_IF_ERR(err);
    }

    /* Get channel */
    ZxChannel *channel;
    err = proc->get_object_with_rights(channel_handle, ZX_RIGHT_WRITE, channel);
    SYS_RET_IF_ERR(err);

    /* Attempt the read */
    bool may_discard = (options & ZX_CHANNEL_READ_MAY_DISCARD);
    err = channel->read_msg(bytes, &num_bytes, handles, &num_handles, may_discard);
    /* Special case for ZX_ERR_BUFFER_TOO_SMALL: return size of next message */
    if (err != ZX_OK && err != ZX_ERR_BUFFER_TOO_SMALL) {
        return err;
    }

    if (actual_bytes != NULL) {
        *actual_bytes = num_bytes;
    }

    if (actual_handles != NULL) {
        *actual_handles = num_handles;
    }

    *num = num_handles;
    return err;
}

uint64_t sys_channel_read(seL4_MessageInfo_t tag, uint64_t badge)
{
    zx_status_t err;
    uint32_t num;
    Handle *h[ZX_CHANNEL_MAX_MSG_HANDLES];
    zx_handle_t *handle_info;

    err = channel_read_common(tag, badge, &h[0], handle_info, &num);
    SYS_RET_IF_ERR(err);

    ZxProcess *proc = get_proc_from_badge(badge);
    for (size_t i = 0; i < num; ++i) {
        /* Cancel waiters on handle */
        h[i]->get_object()->cancel_waiters(h[i]);

        /* Add to proc */
        proc->add_handle(h[i]);

        /* Get user val */
        handle_info[i] = proc->get_handle_user_val(h[i]);
    }

    return ZX_OK;
}

uint64_t sys_channel_read_etc(seL4_MessageInfo_t tag, uint64_t badge)
{
    zx_status_t err;
    uint32_t num;
    Handle *h[ZX_CHANNEL_MAX_MSG_HANDLES];
    zx_handle_info_t *handle_info;

    err = channel_read_common(tag, badge, &h[0], handle_info, &num);
    SYS_RET_IF_ERR(err);

    ZxProcess *proc = get_proc_from_badge(badge);
    for (size_t i = 0; i < num; ++i) {
        /* Cancel waiters on handle */
        h[i]->get_object()->cancel_waiters(h[i]);

        /* Add to proc */
        proc->add_handle(h[i]);

        /* Write to info struct */
        handle_info[i].handle = proc->get_handle_user_val(h[i]);
        handle_info[i].type = h[i]->get_object()->get_object_type();
        handle_info[i].rights = h[i]->get_rights();
        handle_info[i].unused = 0;
    }

    return ZX_OK;
}

namespace SysChannel {

/* Extract handles from proc so they can be written to a channel. */
zx_status_t take_handles_from_proc(ZxProcess *proc, ZxChannel *channel,
        uint32_t num, zx_handle_t *in, Handle **out)
{
    /* Get handle ptrs, validate */
    for (size_t i = 0; i < num; ++i) {
        Handle *h = proc->get_handle(in[i]);
        if (h == NULL) {
            return ZX_ERR_BAD_HANDLE;
        }

        /* Can't write a handle that points to this channel! */
        if (h->get_object() == channel) {
            return ZX_ERR_NOT_SUPPORTED;
        }

        if (!h->has_rights(ZX_RIGHT_TRANSFER)) {
            return ZX_ERR_ACCESS_DENIED;
        }

        out[i] = h;
    }

    /* Remove from proc */
    for (size_t i = 0; i < num; ++i) {
        if (!proc->has_handle(out[i])) {
            /* We've already removed this handles, which means
               there was a duplicate. Add back handles & return. */
            for (size_t j = 0; j < i; ++j) {
                proc->add_handle(out[j]);
            }
            return ZX_ERR_INVALID_ARGS;
        }
        proc->remove_handle(out[i]);
    }

    return ZX_OK;
}

} /* namespace SysChannel */
