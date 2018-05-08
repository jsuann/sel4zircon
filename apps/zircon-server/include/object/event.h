#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

#include "object.h"
#include "waiter.h"

class ZxEvent final : public ZxObjectWaitable {
public:
    ZxEvent() {}
    ~ZxEvent() final {}

    void destroy() override {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_EVENT; }
    CookieJar* get_cookie_jar() override { return &cookie_jar_; }

    zx_status_t user_signal(uint32_t clear_mask, uint32_t set_mask, bool peer) override {
        if (peer) {
            return ZX_ERR_NOT_SUPPORTED;
        }

        constexpr uint32_t kUserSignalMask = ZX_EVENT_SIGNALED | ZX_USER_SIGNAL_ALL;
        if ((set_mask & ~kUserSignalMask) || (clear_mask & ~kUserSignalMask)) {
            return ZX_ERR_INVALID_ARGS;
        }

        update_state(clear_mask, set_mask);
        return ZX_OK;
    }

private:
    CookieJar cookie_jar_;
};

class ZxEventPair final : public ZxObjectWaitable {
friend zx_status_t create_eventpair(ZxEventPair *&ep0, ZxEventPair *&ep1);
public:
    ZxEventPair() {}
    ~ZxEventPair() final {}

    void destroy() override;

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_EVENT_PAIR; }
    CookieJar* get_cookie_jar() override { return &cookie_jar_; }

    zx_status_t user_signal(uint32_t clear_mask, uint32_t set_mask, bool peer) override;

private:
    CookieJar cookie_jar_;
    ZxEventPair *peer_ = NULL;
};

zx_status_t create_eventpair(ZxEventPair *&ep0, ZxEventPair *&ep1);
