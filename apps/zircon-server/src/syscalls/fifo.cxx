#include <autoconf.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"

void sys_fifo_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    uint32_t count = seL4_GetMR(0);
    uint32_t elemsize = seL4_GetMR(1);
    uint32_t options = seL4_GetMR(2);
    uintptr_t user_out0 = seL4_GetMR(3);
    uintptr_t user_out1 = seL4_GetMR(4);

    if (options != 0) {
        return sys_reply(ZX_ERR_INVALID_ARGS);
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge); // TODO policy check

    /* Get ptrs */
    zx_handle_t *out0, *out1;
    err = proc->get_kvaddr(user_out0, out0);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_out1, out1);
    SYS_RET_IF_ERR(err);

    /* Create fifo pair */
    ZxFifo *fifo0, *fifo1;
    err = create_fifo_pair(count, elemsize, fifo0, fifo1);
    SYS_RET_IF_ERR(err);

    /* Create handles to fifos */
    Handle *h0, *h1;
    h0 = create_handle_default_rights(fifo0);
    h1 = create_handle_default_rights(fifo1);
    if (h0 == NULL || h1 == NULL) {
        if (h0 != NULL) {
            fifo0->destroy_handle(h0);
        }
        destroy_object(fifo0);
        destroy_object(fifo1);
        sys_reply(ZX_ERR_NO_MEMORY);
    }

    /* Add handles to proc */
    proc->add_handle(h0);
    proc->add_handle(h1);

    /* Set out vals */
    *out0 = proc->get_handle_user_val(h0);
    *out1 = proc->get_handle_user_val(h1);

    sys_reply(ZX_OK);
}

void sys_fifo_read(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 4);
    zx_handle_t handle = seL4_GetMR(0);
    uintptr_t user_buf = seL4_GetMR(1);
    size_t len = seL4_GetMR(2);
    uintptr_t user_actual = seL4_GetMR(3);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    /* Get ptrs */
    void *buf;
    uint32_t *actual;
    err = proc->uvaddr_to_kvaddr(user_buf, len, buf);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_actual, actual);
    SYS_RET_IF_ERR(err);

    /* Get fifo */
    ZxFifo *fifo;
    err = proc->get_object_with_rights(handle, ZX_RIGHT_READ, fifo);
    SYS_RET_IF_ERR(err);

    /* Write to peer */
    err = fifo->read((uint8_t *)buf, len, actual);
    SYS_RET_IF_ERR(err);

    sys_reply(ZX_OK);
}

void sys_fifo_write(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 4);
    zx_handle_t handle = seL4_GetMR(0);
    uintptr_t user_buf = seL4_GetMR(1);
    size_t len = seL4_GetMR(2);
    uintptr_t user_actual = seL4_GetMR(3);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    /* Get ptrs */
    void *buf;
    uint32_t *actual;
    err = proc->uvaddr_to_kvaddr(user_buf, len, buf);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_actual, actual);
    SYS_RET_IF_ERR(err);

    /* Get fifo */
    ZxFifo *fifo;
    err = proc->get_object_with_rights(handle, ZX_RIGHT_WRITE, fifo);
    SYS_RET_IF_ERR(err);

    if (fifo->get_peer() == NULL) {
        return sys_reply(ZX_ERR_PEER_CLOSED);
    }

    /* Write to peer */
    err = fifo->get_peer()->write((uint8_t *)buf, len, actual);
    SYS_RET_IF_ERR(err);

    sys_reply(ZX_OK);
}
