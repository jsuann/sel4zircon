#include "object/job.h"

namespace JobCxx {
ZxJob *root_job;
};

void init_root_job()
{
    using namespace JobCxx;
    root_job = allocate_object<ZxJob>();
    assert(root_job != NULL);
}

ZxJob *get_root_job()
{
    return JobCxx::root_job;
}

void ZxJob::destroy()
{
    /* Sanity check: job & proc lists should be empty */
    assert(proc_list_.empty());
    assert(job_list_.empty());

    /* We should exit if root job, since we have no parent */
    if (this == JobCxx::root_job) {
        dprintf(CRITICAL, "WARNING: Root job has been destroyed!\n");
        return;
    }

    /* Detach from parent job */
    ZxJob *parent = (ZxJob *)get_owner();
    parent->remove_job(this);

    /* Check if parent should be destroyed */
    if (parent->can_destroy()) {
        return destroy_object(parent);
    }
}

void ZxJob::kill()
{
    /* If already killed, return */
    if (state_ == State::DEAD) {
        return;
    }

    /* Put in killing state. This prevents children
       from destroying us */
    state_ = State::KILLING;

    /* Kill all processes */
    proc_list_.for_each([](ZxProcess * proc) {
        proc->kill();

        if (proc->can_destroy()) {
            destroy_object(proc);
        }
    });

    /* Kill child jobs */
    job_list_.for_each([](ZxJob * job) {
        job->kill();

        if (job->can_destroy()) {
            destroy_object(job);
        }
    });

    update_state(0u, ZX_TASK_TERMINATED);
    state_ = State::DEAD;
}
