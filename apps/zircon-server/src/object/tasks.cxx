#include "object/tasks.h"

void task_kill_thread(ZxThread *thrd)
{
    /* Get the owning process */
    ZxProcess *proc = (ZxProcess *)thrd->get_owner();

    /* We kill the process instead if we have the last
       running thread */
    if (thrd->is_alive() && proc->thread_exited()) {
        return task_kill_process(proc);
    }

    /* Otherwise just kill the thread */
    thrd->kill();

    /* See if thread can be destroyed */
    if (thrd->can_destroy()) {
        destroy_object(thrd);
    }
}

void task_kill_process(ZxProcess *proc)
{
    /* Kill the process */
    proc->kill();

    /* See if proc can be destroyed */
    if (proc->can_destroy()) {
        destroy_object(proc);
    }
}

void task_kill_job(ZxJob *job)
{
    /* Kill the job, check if destroy */
    job->kill();

    if (job->can_destroy()) {
        destroy_object(job);
    }
}

bool task_kill(ZxObject *obj)
{
    zx_obj_type_t type = obj->get_object_type();

    if (type == ZX_OBJ_TYPE_JOB) {
        task_kill_job((ZxJob *)obj);
    } else if (type == ZX_OBJ_TYPE_PROCESS) {
        task_kill_process((ZxProcess *)obj);
    } else if (type == ZX_OBJ_TYPE_THREAD) {
        task_kill_thread((ZxThread *)obj);
    } else {
        return false;
    }

    return true;
}
