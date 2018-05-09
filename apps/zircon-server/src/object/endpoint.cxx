#include "object/endpoint.h"
#include "server.h"

namespace EndpointCxx {

/* This should be small enough so that a full
   table lookup isn't an issue */
constexpr size_t kMaxNumEndpoints = 16;

/* Endpoint lookup table */
ZxEndpoint *ep_table[kMaxNumEndpoints] = {NULL};

}

/*
 * ep table funcs
 */
ZxEndpoint **allocate_ep_slot(uint64_t id)
{
    using namespace EndpointCxx;

    ZxEndpoint **first_free = NULL;
    for (size_t i = 0; i < kMaxNumEndpoints; ++i) {
        /* Check that the id we want to use isn't taken */
        if (ep_table[i] != NULL && ep_table[i]->get_id() == id) {
            return NULL;
        } else if (ep_table[i] == NULL && first_free == NULL) {
            first_free = &ep_table[i];
        }
    }
    return first_free;
}

ZxEndpoint *get_ep_in_table(uint64_t id)
{
    using namespace EndpointCxx;

    for (size_t i = 0; i < kMaxNumEndpoints; ++i) {
        if (ep_table[i] != NULL && ep_table[i]->get_id() == id) {
            return ep_table[i];
        }
    }
    return NULL;
}

void free_ep_slot(ZxEndpoint *ep)
{
    using namespace EndpointCxx;

    for (size_t i = 0; i < kMaxNumEndpoints; ++i) {
        if (ep_table[i] == ep) {
            ep_table[i] = NULL;
            return;
        }
    }
}

/*
 * ZxEndpoint member funcs
 */
bool ZxEndpoint::init()
{
    int error;
    vka_t *vka = get_server_vka();

    error = vka_alloc_endpoint(vka, &ep_);
    if (error) {
        return false;
    }

    return true;
}

void ZxEndpoint::destroy()
{
    vka_t *vka = get_server_vka();

    if (ep_.cptr != 0) {
        /* Revoke all ep caps */
        cspacepath_t src;
        vka_cspace_make_path(vka, ep_.cptr, &src);
        vka_cnode_revoke(&src);
        /* Free the ep object */
        vka_free_object(vka, &ep_);
    }

    free_ep_slot(this);
}
