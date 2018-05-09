#include <autoconf.h>

extern "C" {
#include "debug.h"
#include <vka/vka.h>
}

#include "sys_helpers.h"
#include "object/endpoint.h"

uint64_t sys_endpoint_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 4);
    zx_handle_t hrsrc = seL4_GetMR(0);
    uint64_t id = seL4_GetMR(1);
    uint32_t options = seL4_GetMR(2);
    uintptr_t user_out = seL4_GetMR(3);

    if (options & ~ZX_ENDPOINT_EXTERN_CREATOR) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    /* Check the process has the correct resource. For now
       this is just the root resource. */
    err = validate_resource(proc, hrsrc, ZX_RSRC_KIND_ROOT);
    SYS_RET_IF_ERR(err);

    zx_handle_t *out;
    err = proc->get_kvaddr(user_out, out);
    SYS_RET_IF_ERR(err);

    if (options & ZX_ENDPOINT_EXTERN_CREATOR) {
        /* Endpoint was created by a native seL4
           process. Find it with id. */
        ZxEndpoint *ep = get_ep_in_table(id);
        if (ep == NULL) {
            return ZX_ERR_NOT_FOUND;
        }

        /* Check the ep was created externally */
        if (!ep->extern_creator()) {
            return ZX_ERR_BAD_STATE;
        }

        zx_handle_t ep_handle = proc->create_handle_get_uval(ep);
        if (ep_handle == ZX_HANDLE_INVALID) {
            return ZX_ERR_NO_MEMORY;
        }

        *out = ep_handle;
        return ZX_OK;
    }

    /* Allocate a slot in the ep table */
    ZxEndpoint **ep_slot;
    ep_slot = allocate_ep_slot(id);
    if (ep_slot == NULL) {
        return ZX_ERR_NO_RESOURCES;
    }

    /* Create the ep object */
    ZxEndpoint *ep = allocate_object<ZxEndpoint>(id, false);
    if (ep == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Init the ep */
    if (!ep->init()) {
        free_object(ep);
        return ZX_ERR_NO_MEMORY;
    }

    zx_handle_t ep_handle = proc->create_handle_get_uval(ep);
        if (ep_handle == ZX_HANDLE_INVALID) {
        destroy_object(ep);
        return ZX_ERR_NO_MEMORY;
    }

    *ep_slot = ep;
    *out = ep_handle;
    return ZX_OK;
}

uint64_t sys_endpoint_mint_cap(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t ep_handle = seL4_GetMR(0);
    zx_handle_t thrd_handle = seL4_GetMR(1);
    seL4_CPtr ep_slot = seL4_GetMR(2);
    seL4_Word ep_badge = seL4_GetMR(3);
    seL4_Word ep_rights = seL4_GetMR(4);

    /* Check validity of slot. Note that this only checks that the slot is
       within the cspace, and is not reserved by the server. Free slots are to
       be managed by the caller. */
    if (ep_slot < ZX_THREAD_FIRST_FREE || ep_slot >= ZX_THREAD_NUM_CSPACE_SLOTS) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Check validity of badge. For now, we don't accept zero badges. */
    if (ep_badge == 0 || ep_badge >= (1 << seL4_BadgeBits)) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    /* Get endpoint & thread objects */
    ZxEndpoint *ep;
    ZxThread *target_thrd;
    /* For now, both ep and thread are expected to have ZX_RIGHT_WRITE to
       permit a write to the cspace */
    err = proc->get_object_with_rights(ep_handle, ZX_RIGHT_WRITE, ep);
    SYS_RET_IF_ERR(err);
    err = proc->get_object_with_rights(thrd_handle, ZX_RIGHT_WRITE, target_thrd);
    SYS_RET_IF_ERR(err);

    /* Get the rights */
    bool can_read = (ep_rights & ZX_ENDPOINT_CAN_READ);
    bool can_write = (ep_rights & ZX_ENDPOINT_CAN_WRITE);
    bool can_grant = (ep_rights & ZX_ENDPOINT_CAN_GRANT);

    /* Get a path to the ep cap, and mint to thread cspace. If this
       fails, we assume it is due to the slot already being used. */
    cspacepath_t src;
    vka_cspace_make_path(get_server_vka(), ep->get_ep_cap(), &src);
    err = target_thrd->mint_cap(&src, ep_slot, ep_badge,
            seL4_CapRights_new(can_grant, can_read, can_write));
    if (err) {
        dprintf(INFO, "mint cap returned %d\n", err);
        return ZX_ERR_BAD_STATE;
    }

    return ZX_OK;
}

uint64_t sys_endpoint_delete_cap(seL4_MessageInfo_t tag, uint64_t badge)
{
    return ZX_ERR_NOT_SUPPORTED;
}
