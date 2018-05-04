#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

#include "object.h"

class ZxEvent final : public ZxObject {
public:
    ZxEvent() : waiter_list_{this} {}
    ~ZxEvent() final {}

    void destroy() override;

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_EVENT; }
    virtual bool has_state_tracker() const { return true; }
    virtual CookieJar* get_cookie_jar() { return &cookie_jar_; }

private:
    LinkedList<StateWaiter> waiter_list_;
    CookieJar cookie_jar_;
};

class ZxEventPair final : public ZxObject {
friend zx_status_t create_eventpair(ZxEventPair *&ep0, ZxEventPair *&ep1);
public:
    ZxEventPair() : waiter_list_{this} {}
    ~ZxEventPair() final {}

    void destroy() override;

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_EVENT_PAIR; }
    virtual bool has_state_tracker() const { return true; }
    virtual CookieJar* get_cookie_jar() { return &cookie_jar_; }

private:
    LinkedList<StateWaiter> waiter_list_;
    CookieJar cookie_jar_;
    ZxEventPair *peer_ = NULL;
};

zx_status_t create_eventpair(ZxEventPair *&ep0, ZxEventPair *&ep1);
