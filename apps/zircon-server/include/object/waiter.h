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

#define WAITER_TYPE_NONE        0
#define WAITER_TYPE_STATE       1
#define WAITER_TYPE_PORT        2
#define WAITER_TYPE_CHANNEL     3

class Waiter {
public:
    Waiter(ZxThread *thrd) : thrd_{thrd} {}
    ~Waiter() {}

    virtual uint32_t type() const { return WAITER_TYPE_NONE; }

    ZxThread *get_thread() const { return thrd_; }

private:
    /* The waiting thread */
    ZxThread *thrd_;
};

/* Waits for state change of an object */
class StateWaiter final : public Waiter, public Listable<StateWaiter>  {
public:
    StateWaiter(ZxThread *thrd) : Waiter(thrd) {}
    ~StateWaiter() {}

    virtual uint32_t type() const { return WAITER_TYPE_STATE; }

    bool init(zx_signals_t initial_state, Handle *h) {
        handle_ = h;
        wakeup_reasons_ = watched_signals_ = initial_state;
        return (wakeup_reasons_ & watched_signals_) ? true : false;
    }

    bool state_change(zx_signals_t new_state) {
        wakeup_reasons_ |= new_state;
        return (wakeup_reasons_ & watched_signals_) ? true : false;
    }

    bool cancel(Handle *h);

private:
    Handle *handle_ = NULL;
    zx_signals_t watched_signals_ = 0u;
    zx_signals_t wakeup_reasons_ = 0u;
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
