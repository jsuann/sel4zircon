#include "object/timer.h"

void ZxTimer::destroy()
{
    /* If timer pending, remove it */
    if (timer_.is_waiting()) {
        remove_timer(&timer_);
    }
}

zx_status_t ZxTimer::set(zx_time_t deadline, zx_duration_t slack)
{
    /* Check if deadline has already happened */
    if (deadline == 0u || deadline <= get_system_time()) {
        update_state(0u, ZX_TIMER_SIGNALED);
        return ZX_OK;
    }

    /* If timer already set, remove it first */
    if (timer_.is_waiting()) {
        remove_timer(&timer_);
    }

    /* Set the timer */
    timer_.set_callback(timer_fired_cb, (void *)this);
    add_timer(&timer_, deadline, slack, flags_);
    return ZX_OK;
}

zx_status_t ZxTimer::cancel()
{
    /* Always clear the signal bit */
    update_state(ZX_TIMER_SIGNALED, 0u);

    /* If timer is pending remove it */
    if (timer_.is_waiting()) {
        remove_timer(&timer_);
    }

    return ZX_OK;
}

void timer_fired_cb(void *data)
{
    ZxTimer *t = (ZxTimer *)data;
    t->update_state(0u, ZX_TIMER_SIGNALED);
}
