#include "object/waiter.h"

void ZxObjectWaitable::update_waiters(zx_signals_t signals)
{
    size_t num = waiter_list_.size();
    /* Loop through all waiters. If we get a signal match, notify thread/port.
       Else put waiter back on list. */
    for (size_t i = 0; i < num; ++i) {
        StateWaiter *sw = waiter_list_.pop_front();
        if (sw->state_change(signals)) {
            if (sw->get_observer()->get_object_type() == ZX_OBJ_TYPE_THREAD) {
                /* Thread always wakes up if a watched signal is observed */
                sw->get_thread()->obj_wait_resume(sw, ZX_OK);
            } else {
                /* Have port. Notify that packet should be delivered to port */
                /* If type is ZX_WAIT_ASYNC_REPEATING, we reinsert */
            }
        } else {
            waiter_list_.push_back(sw);
        }
    }
}

void ZxObjectWaitable::cancel_waiters(Handle *h)
{
    size_t num = waiter_list_.size();
    for (size_t i = 0; i < num; ++i) {
        StateWaiter *sw = waiter_list_.pop_front();
        if (h == sw->get_handle()) {
            if (sw->get_observer()->get_object_type() == ZX_OBJ_TYPE_THREAD) {
                /* If thread, cancel the wait. */
                sw->get_thread()->obj_wait_resume(sw, ZX_ERR_CANCELED);
            } else {
                /* If port, keep packet in queue if added, but cancel updating */
            }
        } else {
            waiter_list_.push_back(sw);
        }
    }
}
