#pragma once

#include <autoconf.h>

extern "C" {
#include <zircon/types.h>
#include <sel4/sel4.h>
#include <sel4platsupport/timer.h>
#include "debug.h"
}

typedef void (*timer_callback_func)(void *data);

/* Base class for objects that can wait */
class TimerNode {

/* Timer funcs are all friends */
/*
friend void add_timer(TimerNode *t, uint64_t expire_time, uint32_t flags);
friend void handle_timer(seL4_Word badge);
friend void remove_timer(TimerNode *t);
friend bool has_timer_expired(TimerNode *t, uint64_t time, uint64_t slack);
*/

public:
    TimerNode() = default;
    ~TimerNode() {}

    virtual void timer_callback();

    TimerNode *get_next() const { return next_ };
    uint64_t get_expire_time() const { return expire_time_ };
    bool get_slack_early() const { return slack_early_ };
    bool is_waiting() const { return waiting_; }

    void set_next(TimerNode *next) { next_ = next; }
    void set_expire_time(uint64_t expire_time) { expire_time_ = expire_time; }
    void set_slack_early(bool slack_early) { slack_early_ = slack_early; }
    void set_waiting(bool waiting) { waiting_ = waiting; }

private:
    TimerNode *next_ = NULL;
    uint64_t expire_time_ = 0;
    bool slack_early_ = false;
    bool waiting_ = false;
};


void init_timer(seL4_timer_t *timer, seL4_CPtr ntfn,
        seL4_CPtr server_tcb, seL4_Word *timer_badge);

bool has_timer_expired(TimerNode *t, uint64_t time, uint64_t slack);

void handle_timer(seL4_Word badge);

void add_timer(TimerNode *t, uint64_t expire_time, uint32_t flags);

void remove_timer(TimerNode *t);
