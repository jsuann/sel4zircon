#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

#include "object.h"
#include "linkedlist.h"
#include "process.h"

class ZxJob final : public ZxObject, public Listable<ZxJob> {
public:
    ZxJob() : job_list_{this}, proc_list_{this} {}

    bool can_destroy() override {
        /* End of life when no child jobs or procs */
        return (zero_handles() && job_list_.empty() && proc_list_.empty());
    }

    void destroy() override {}

    void add_process(ZxProcess *proc) {
        proc_list_.push_back(proc);
    }

    void add_job(ZxJob *job) {
        job_list_.push_back(job);
    }

private:
    /* XXX We don't deal with job importance currently. */

    /* TODO policies? */

    /* Exception port */

    LinkedList<ZxJob> job_list_;
    LinkedList<ZxProcess> proc_list_;
};

void init_root_job();
ZxJob *get_root_job();
