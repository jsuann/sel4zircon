#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <zircon/types.h>
}

#include "object.h"
#include "linkedlist.h"
#include "process.h"

class ZxJob final : public ZxObjectWaitable, public Listable<ZxJob> {
public:
    ZxJob() : ZxObjectWaitable(ZX_JOB_NO_PROCESSES | ZX_JOB_NO_JOBS),
        job_list_{this}, proc_list_{this} {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_JOB; }

    bool can_destroy() override {
        /* End of life when no child jobs or procs, and we aren't killing them */
        return (zero_handles() && job_list_.empty() &&
                        proc_list_.empty() && !is_killing());
    }

    void destroy() override;
    void kill();

    enum class State {
        READY,      /* Can add child procs/jobs */
        KILLING,    /* Job is currently killing off jobs/procs */
        DEAD,       /* Job killed, no more jobs/procs */
    };

    bool is_killing() const { return state_ == State::KILLING; }
    bool is_dead() const { return state_ == State::DEAD; }

    void add_process(ZxProcess *proc) {
        proc_list_.push_back(proc);

        if (proc_list_.size() == 1) {
            update_state(ZX_JOB_NO_PROCESSES, 0u);
        }
    }

    void add_job(ZxJob *job) {
        job_list_.push_back(job);

        if (job_list_.size() == 1) {
            update_state(ZX_JOB_NO_JOBS, 0u);
        }
    }

    void remove_process(ZxProcess *proc) {
        proc_list_.remove(proc);

        if (proc_list_.size() == 0) {
            update_state(0u, ZX_JOB_NO_PROCESSES);
        }
    }

    void remove_job(ZxJob *job) {
        job_list_.remove(job);

        if (job_list_.size() == 0) {
            update_state(0u, ZX_JOB_NO_JOBS);
        }
    }

private:
    /* We don't deal with job importance currently, nor policies. */

    /* TODO Exception port */

    State state_ = State::READY;

    LinkedList<ZxJob> job_list_;
    LinkedList<ZxProcess> proc_list_;
};

void init_root_job();
ZxJob *get_root_job();
