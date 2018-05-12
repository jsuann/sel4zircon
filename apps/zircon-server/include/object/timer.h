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
#include "../utils/clock.h"

class ZxTimer final : public ZxObjectWaitable {
public:
    ZxTimer(uint32_t flags) : flags_{flags} {}
    ~ZxTimer() final {}

    void destroy() override;

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_TIMER; }

    zx_status_t set(zx_time_t deadline, zx_duration_t slack);
    zx_status_t cancel();

private:
    TimerNode timer_;
    uint32_t flags_;
};

void timer_fired_cb(void *data);
