#include "utils/clock.h"

namespace ClockCxx {

/* On x86_64 this should normally use hpet */
seL4_timer_t *server_timer;

/* Last timestamp */
uint64_t curr_time = 0;

/* Head of timer list */
TimerNode *head = NULL;

/* Amount of slack */
constexpr uint64_t kTimerSlack = 500 * NS_IN_US;

void update_timeout(uint64_t expire_time) {
    /* Try to make sure next timeout isn't too soon */
    ltimer_get_time(&server_timer->ltimer, &curr_time);
    uint64_t min_time = curr_time + kTimerSlack;
    uint64_t next_timeout = (expire_time > min_time) ? expire_time : min_time;

    /* Set the next timeout */
    if (ltimer_set_timeout(&server_timer->ltimer, next_timeout, TIMEOUT_ABSOLUTE) != 0) {
        dprintf(CRITICAL, "Error setting timeout!\n");
    }
}

} /* namespace ClockCxx */

void init_timer(seL4_timer_t *timer, seL4_CPtr ntfn,
        seL4_CPtr server_tcb, seL4_Word *timer_badge)
{
    using namespace ClockCxx;

    /* XXX newer versions of QEMU seem to constantly fire interrupts, in addition
       to when we program the hpet to fire. It's not too much of an issue since
       we don't use TIMEOUT_PERIODIC, which constantly reprograms hpet, causing
       it to never actually fire correctly in QEMU! */

    /* get the timer badge with a quick timeout. */
    assert(ltimer_set_timeout(&timer->ltimer, kTimerSlack, TIMEOUT_RELATIVE) == 0);
    seL4_Wait(ntfn, timer_badge);
    sel4platsupport_handle_timer_irq(timer, *timer_badge);
    dprintf(INFO, "Timer badge: %lu\n", *timer_badge);

    /* Bind notification to the tcb */
    assert(seL4_TCB_BindNotification(server_tcb, ntfn) == 0);

    /* Store server timer */
    server_timer = timer;
}

bool has_timer_expired(TimerNode *t, uint64_t time, uint64_t slack)
{
    return (t != NULL && t->expire_time_ <= (time + slack));
}

void handle_timer(seL4_Word badge)
{
    using namespace ClockCxx;

    /* Get time */
    ltimer_get_time(&server_timer->ltimer, &curr_time);

    /* Ack the timer irq */
    sel4platsupport_handle_timer_irq(server_timer, badge);

    /* Pop any expired timers */
    bool progress = false;
    for (;;) {
        TimerNode *t = head;
        /* Check if null, or not expired with slack */
        if (!(has_timer_expired(t, curr_time, kTimerSlack))) {
            break;
        }
        /* If timer can go late, check without slack */
        if (!t->slack_early_ && !(has_timer_expired(t, curr_time, 0))) {
            break;
        }

        /* Timer has expired, remove from list */
        head = t->next_;
        t->next_ = NULL;
        t->expire_time_ = 0;
        t->slack_early_ = false;
        t->waiting_ = false;

        /* do callback */
        t->timer_callback();

        progress = true;
    }

    /* Update timeout if head changed */
    if (progress && head != NULL) {
        update_timeout(head->expire_time_);
    }
}

void add_timer(TimerNode *t, uint64_t expire_time, uint32_t flags)
{
    using namespace ClockCxx;

    /* Set up timer args */
    t->expire_time_ = expire_time;
    t->waiting_ = true;
    if (flags == ZX_TIMER_SLACK_EARLY || flags == ZX_TIMER_SLACK_CENTER) {
        t->slack_early_ = true;
    }

    /* Insert in order in timer list */
    TimerNode *curr;
    TimerNode **pt = &head;
    for (;;) {
        curr = *pt;
        if (!(has_timer_expired(curr, expire_time, 0))) {
            break;
        }
        pt = &curr->next_;
    }
    t->next_ = *pt;
    *pt = t;

    /* If new timer at head, update the timeout */
    if (t == head) {
        update_timeout(t->expire_time_);
    }
}

void remove_timer(TimerNode *t)
{
    using namespace ClockCxx;

    /* Remove from list */
    TimerNode *curr;
    TimerNode **pt = &head;
    for (;;) {
        curr = *pt;
        if (curr == NULL) {
            assert(!"Attempt to remove timer not in timer list!");
        }
        if (t == curr) {
            *pt = t->next_;
        }
        pt = &curr->next_;
    }

    /* Reset */
    t->next_ = NULL;
    t->expire_time_ = 0;
    t->slack_early_ = false;
    t->waiting_ = false;
}
