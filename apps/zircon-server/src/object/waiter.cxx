#include "object/waiter.h"

void update_state_waiters(LinkedList<StateWaiter> &waiter_list, zx_signals_t signals)
{
    size_t num = waiter_list.size();

    /* Loop through all waiters. If we get a signal match, notify thread/port.
       Else put waiter back on list. */
    for (size_t i = 0; i < num; ++i) {
        StateWaiter *sw = waiter_list.pop_front();
        bool reinsert = true;
        if (sw->get_observer()->get_object_type() == ZX_OBJ_TYPE_THREAD) {
            if (sw->state_change(signals)) {
                /* Thread always wakes up if a watched signal is observed */
                sw->get_thread()->signal_observed(sw);
                reinsert = false;
            }
        } else {
            /* Have port. Notify that packet should be delivered to port */
            // TODO
            /* If type is ZX_WAIT_ASYNC_ONCE, we don't reinsert */
        }
        if (reinsert) {
            waiter_list.push_back(sw);
        }
    }
}
