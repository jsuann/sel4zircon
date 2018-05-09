#include <autoconf.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"
#include "object/socket.h"

uint64_t sys_socket_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 3);
    uint32_t options = seL4_GetMR(0);
    uintptr_t user_out0 = seL4_GetMR(1);
    uintptr_t user_out1 = seL4_GetMR(2);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    zx_handle_t *out0, *out1;
    err = proc->get_kvaddr(user_out0, out0);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_out1, out1);
    SYS_RET_IF_ERR(err);

    /* Create socket pair */
    ZxSocket *sock0, *sock1;
    err = create_socket_pair(sock0, sock1);
    SYS_RET_IF_ERR(err);

    /* Create handles to sockets */
    Handle *h0, *h1;
    h0 = create_handle_default_rights(sock0);
    h1 = create_handle_default_rights(sock1);
    if (h0 == NULL || h1 == NULL) {
        if (h0 != NULL) {
            sock0->destroy_handle(h0);
        }
        destroy_object(sock0);
        destroy_object(sock1);
        return ZX_ERR_NO_MEMORY;
    }

    /* Add handles to proc, set out vals */
    proc->add_handle(h0);
    proc->add_handle(h1);
    *out0 = proc->get_handle_user_val(h0);
    *out1 = proc->get_handle_user_val(h1);
    return ZX_OK;
}

uint64_t sys_socket_write(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t socket_handle = seL4_GetMR(0);
    uint32_t options = seL4_GetMR(1);
    uintptr_t user_buffer = seL4_GetMR(2);
    size_t size = seL4_GetMR(3);
    uintptr_t user_actual = seL4_GetMR(4);

    if (size > 0 && user_buffer == 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    /* Get socket */
    ZxSocket *socket;
    err = proc->get_object_with_rights(socket_handle, ZX_RIGHT_WRITE, socket);
    SYS_RET_IF_ERR(err);

    /* Check for shutdown request */
    if (options & ZX_SOCKET_SHUTDOWN_MASK) {
        if (size != 0) {
            return ZX_ERR_INVALID_ARGS;
        }
        return socket->shutdown(options & ZX_SOCKET_SHUTDOWN_MASK);
    }

    void* buffer;
    size_t* actual = NULL;
    err = proc->uvaddr_to_kvaddr(user_buffer, size, buffer);
    SYS_RET_IF_ERR(err);
    if (user_actual != 0) {
        err = proc->get_kvaddr(user_actual, actual);
        SYS_RET_IF_ERR(err);
    }

    size_t nwritten;
    if (options == ZX_SOCKET_CONTROL) {
        err = socket->write_control(buffer, size);
        SYS_RET_IF_ERR(err);
        nwritten = size;
    } else if (options == 0) {
        err = socket->write(buffer, size, &nwritten);
        SYS_RET_IF_ERR(err);
    } else {
        return ZX_ERR_INVALID_ARGS;
    }

    if (actual != NULL) {
        *actual = nwritten;
    }

    return ZX_OK;
}

uint64_t sys_socket_write(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t handle = seL4_GetMR(0);
    uint32_t options = seL4_GetMR(1);
    uintptr_t user_buffer = seL4_GetMR(2);
    size_t size = seL4_GetMR(3);
    uintptr_t user_actual = seL4_GetMR(4);

    if (size > 0 && user_buffer == 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    void* buffer;
    size_t* actual = NULL;
    err = proc->uvaddr_to_kvaddr(user_buffer, size, buffer);
    SYS_RET_IF_ERR(err);
    if (user_actual != 0) {
        err = proc->get_kvaddr(user_actual, actual);
        SYS_RET_IF_ERR(err);
    }

    ZxSocket *socket;
    err = proc->get_object_with_rights(socket_handle, ZX_RIGHT_WRITE, socket);
    SYS_RET_IF_ERR(err);

    size_t nread;
    switch (options) {
    case 0:
        err = socket->read(buffer, size, &nread);
    case ZX_SOCKET_CONTROL:
        err = socket->read_control(buffer, size, &nread);
    default:
        return ZX_ERR_INVALID_ARGS;
    }

    if (err == ZX_OK && actual != NULL) {
        *actual = nwritten;
    }

    return err;
}
