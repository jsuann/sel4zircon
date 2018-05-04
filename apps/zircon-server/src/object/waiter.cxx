#include "object/waiter.h"

void update_state_waiters(LinkedList<StateWaiter> &waiter_list, zx_signals_t signals)
{
    size_t num = waiter_list.size();

    /* Loop through all waiters. If we get a signal match, notify thread.
       Else put waiter back on list. */
    for (size_t i = 0; i < num; ++i) {
        StateWaiter *sw = waiter_list.pop_front();
        if (sw->state_change(signals)) {
            sw->get_thread()->signal_observed(sw);
        } else {
            waiter_list.push_back(sw);
        }
    }
}
