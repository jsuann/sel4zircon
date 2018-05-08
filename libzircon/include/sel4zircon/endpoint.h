#pragma once

#include <autoconf.h>
#include <sel4/sel4.h>
#include <zircon/rights.h>

/* Special syscalls for providing sel4zircon processes
   access to seL4 endpoints */

/* Create an endpoint, with an associated lookup ID. Requires
   a suitable resource object to authorise creation. */
extern zx_status_t zx_endpoint_create(zx_handle_t resource,
        uint64_t id, zx_handle_t *out);

/* Mint a capability to an endpoint to a thread's cspace.
   Also sets the badge & rights of the endpoint capability. */
extern zx_status_t zx_endpoint_mint_cap(zx_handle_t endpoint,
        zx_handle_t thread, seL4_CPtr slot, seL4_Word badge, seL4_Word rights);

/* Delete an endpoint capability in a thread's cspace. */
extern zx_status_t zx_endpoint_delete_cap(zx_handle_t endpoint,
        zx_handle_t thread, seL4_CPtr slot);

/* Zircon-style rights to an endpoint. In this case, ZX_RIGHT_WRITE
   dictates the ability to mint/delete a cap (a "write" to the cspace). */
#define ZX_DEFAULT_ENDPOINT_RIGHTS \
    (ZX_RIGHT_DUPLICATE | ZX_RIGHT_TRANSFER | ZX_RIGHT_WRITE)

/* Rights of endpoint capabilities. Sets the relevant field in seL4_CapRights_t */
#define ZX_ENDPOINT_CAN_READ    (1u << 0)
#define ZX_ENDPOINT_CAN_WRITE   (1u << 1)
#define ZX_ENDPOINT_CAN_GRANT   (1u << 2)

/* Label for call to transfer an endpoint cap */
#define ZX_SERVER_TRANSFER_EP_CAP    10
