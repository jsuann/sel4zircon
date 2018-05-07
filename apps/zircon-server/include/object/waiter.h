#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

#include "linkedlist.h"

class ZxThread;
class ZxPort;
struct PortPacket;

#define WAITER_TYPE_NONE        0
#define WAITER_TYPE_STATE       1
#define WAITER_TYPE_PORT        2
#define WAITER_TYPE_CHANNEL     3

class Waiter {
public:
    Waiter(ZxObject *observer) : observer_{observer} {}
    ~Waiter() {}

    virtual uint32_t type() const { return WAITER_TYPE_NONE; }

    ZxObject *get_observer() const { return observer_; }

    /* Note: should check for correct object type before calling! */
    ZxThread *get_thread() const { return (ZxThread *)observer_; }
    ZxPort *get_port() const { return (ZxPort *)observer_; }

private:
    /* The waiting object (thread/port) */
    ZxObject *observer_;
};

/* Waits for state change of an object */
class StateWaiter final : public Waiter, public Listable<StateWaiter> {
public:
    StateWaiter(ZxObject *observer) : Waiter(observer) {}
    ~StateWaiter() {}

    virtual uint32_t type() const { return WAITER_TYPE_STATE; }

    Handle *get_handle() const { return handle_; }
    zx_signals_t get_observed() const { return observed_; }
    void *get_data() const { return data_; }

    void set_data(void *data) { data_ = data; }

    bool init_wait(zx_signals_t watching, zx_signals_t initial_state, Handle *h) {
        handle_ = h;
        watching_ = watching;
        observed_ = initial_state;
        return (observed_ & watching_) ? true : false;
    }

    bool state_change(zx_signals_t new_state) {
        observed_ |= new_state;
        return (new_state & watching_) ? true : false;
    }

private:
    /* Handle to object we are watching */
    Handle *handle_ = NULL;
    /* Signals we are waiting for */
    zx_signals_t watching_ = 0u;
    /* Signals we have observed */
    zx_signals_t observed_ = 0u;

    /* For threads, data stores ptr to return val(s).
       For ports, this is a ptr to a PortPacket. */
    void *data_;
};

/* Waits for packet delivered to a port */
class PortWaiter final : public Waiter, public Listable<PortWaiter> {
    virtual uint32_t type() const { return WAITER_TYPE_PORT; }
};

/* Waits for message in channel */
class ChannelWaiter final : public Waiter, public Listable<ChannelWaiter> {
    virtual uint32_t type() const { return WAITER_TYPE_CHANNEL; }
};

void update_state_waiters(LinkedList<StateWaiter> &waiter_list, zx_signals_t signals);


/* Objects that can be waited on for signal assertions changes */
class ZxObjectWaitable : public ZxObject {
public:
    ZxObjectWaitable(zx_signals_t signals = 0u) :
        ZxObject(signals), waiter_list_{this} {}

    bool has_state_tracker() const final { return true; }
    void update_waiters(zx_signals_t signals) final;
    void cancel_waiters(Handle *h) final;

    void add_waiter(StateWaiter *sw) {
        waiter_list_.push_back(sw);
    }

    void remove_waiter(StateWaiter *sw) {
        waiter_list_.remove(sw);
    }

private:
    LinkedList<StateWaiter> waiter_list_;
};

static inline void add_waiter_to_object(StateWaiter *sw, Handle *h)
{
    ZxObjectWaitable *o = (ZxObjectWaitable *)h->get_object();
    o->add_waiter(sw);
}
