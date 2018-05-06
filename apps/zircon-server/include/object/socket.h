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

class ZxSocket final : public ZxObjectWaitable {
friend zx_status_t create_socket_pair(uint32_t flags, ZxSocket *&sock0,
        ZxSocket *&sock1);
public:
    ZxSocket(zx_signals_t starting_signals, uint32_t flags) :
            ZxObjectWaitable(starting_signals), flags_{flags}, datagrams_{this} {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_SOCKET; }

    void destroy() override;

    /* read/write to socket buffer */
    zx_status_t write(void *src, size_t len, size_t* written);
    zx_status_t read(void *dest, size_t len, size_t* nread);

    /* read/write to control plane */
    zx_status_t write_control(void *src, size_t len);
    zx_status_t read_control(void *dest, size_t len, size_t* nread);

    /* share/accept socket handle */
    zx_status_t share(Handle *h);
    zx_status_t accept(Handle **h);

    zx_status_t shutdown(uint32_t how);
    zx_status_t shutdown_by_peer(uint32_t how);

    struct ControlMsg {
        static constexpr size_t kSize = 1024;
        char msg_[kSize];
    };

private:
    struct Datagram : public Listable<Datagram> {
        Datagram(size_t len) : len_{len} {}
        size_t len_;
    };

    MBuf data_buf_;
    const uint32_t flags_;
    bool read_disabled_ = false;

    /* Used if a datagram socket */
    LinkedList<Datagram> datagrams_;

    /* Msg for control sockets */
    ControlMsg *control_msg_ = NULL;
    size_t control_msg_len_;

    /* Handle to socket to be shared */
    Handle *accept_queue_ = NULL;

    ZxSocket *peer_ = NULL;
};

zx_status_t create_socket_pair(uint32_t flags, ZxSocket *&sock0,
        ZxSocket *&sock1);
