#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <zircon/types.h>
#include <sel4zircon/endpoint.h>
}

#include "object.h"

class ZxEndpoint final : public ZxObject {
public:
    ZxEndpoint(uint64_t id) : id_{id} {}
    ~ZxEndpoint() final {}

    bool init();
    void destroy() override;

    seL4_CPtr get_ep_cap() const { return ep_.cptr; }
    uint64_t get_id() const { return id_; }

private:
    vka_object_t ep_ = {0};
    uint64_t id_;
};

/* ep table funcs */
ZxEndpoint **allocate_ep_slot(uint64_t id);
ZxEndpoint *get_ep_in_table(uint64_t id);
void free_ep_slot(ZxEndpoint *ep);

/* Transfer ep cap to native seL4 process */
void transfer_ep_cap(uint64_t id);
