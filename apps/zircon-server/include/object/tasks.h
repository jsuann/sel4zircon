#pragma once

#include "job.h"

/* Task (job, process, thread) helper funcs */
void task_kill_thread(ZxThread *thrd);
void task_kill_process(ZxProcess *proc);
void task_kill_job(ZxJob *job);
bool task_kill(ZxObject *obj);
