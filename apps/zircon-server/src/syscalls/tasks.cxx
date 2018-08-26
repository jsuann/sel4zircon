#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"
#include "object/tasks.h"

namespace SysTasks {
constexpr size_t kMaxDebugReadBlock = 64 * 1024u * 1024u;
constexpr size_t kMaxDebugWriteBlock = 64 * 1024u * 1024u;
}

/* Job syscalls */

uint64_t sys_job_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 6);
    zx_handle_t parent_handle = seL4_GetMR(0);
    uint32_t options = seL4_GetMR(1);
    uintptr_t user_out = seL4_GetMR(2);

    if (options != 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    zx_handle_t *out;
    err = proc->get_kvaddr(user_out, out);
    SYS_RET_IF_ERR(err);

    ZxJob *parent_job;
    err = proc->get_object_with_rights(parent_handle, ZX_RIGHT_WRITE, parent_job);
    SYS_RET_IF_ERR(err);

    /* We can't add new jobs to a dead parent */
    if (parent_job->is_dead()) {
        return ZX_ERR_BAD_STATE;
    }

    ZxJob *new_job = allocate_object<ZxJob>();

    if (new_job == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    zx_handle_t job_handle = proc->create_handle_get_uval(new_job);

    if (job_handle == ZX_HANDLE_INVALID) {
        destroy_object(new_job);
        return ZX_ERR_NO_MEMORY;
    }

    /* Add new job to parent */
    parent_job->add_job(new_job);

    *out = job_handle;
    return ZX_OK;
}

/* Process syscalls */

uint64_t sys_process_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 6);
    zx_handle_t job_handle = seL4_GetMR(0);
    uintptr_t user_buf = seL4_GetMR(1);
    uint32_t name_len = seL4_GetMR(2);
    uint32_t options = seL4_GetMR(3);
    uintptr_t user_proc_handle = seL4_GetMR(4);
    uintptr_t user_vmar_handle = seL4_GetMR(5);

    if (options != 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    void *name_buf;
    zx_handle_t *proc_handle, *vmar_handle;
    err = proc->uvaddr_to_kvaddr(user_buf, name_len, name_buf);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_proc_handle, proc_handle);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_vmar_handle, vmar_handle);
    SYS_RET_IF_ERR(err);

    /* Get the job we are adding the process to */
    ZxJob *job;
    err = proc->get_object_with_rights(job_handle, ZX_RIGHT_WRITE, job);
    SYS_RET_IF_ERR(err);

    /* Create the root vmar */
    ZxVmar *root_vmar;
    root_vmar = allocate_object<ZxVmar>();

    if (root_vmar == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Create the new process */
    ZxProcess *new_proc;
    new_proc = allocate_object<ZxProcess>(root_vmar);

    if (new_proc == NULL) {
        free_object(root_vmar);
        return ZX_ERR_NO_MEMORY;
    }

    new_proc->set_name((char *)name_buf);

    if (!new_proc->init()) {
        /* This will also destroy vmar */
        destroy_object(new_proc);
        return ZX_ERR_NO_MEMORY;
    }

    /* Create the handles. Default vmar rights are
       augmented with map permission rights */
    Handle *ph, *vh;
    vh = root_vmar->create_handle(root_vmar->get_rights());
    ph = create_handle_default_rights(new_proc);

    if (vh == NULL || ph == NULL) {
        if (vh != NULL) {
            root_vmar->destroy_handle(vh);
        }

        destroy_object(new_proc);
        return ZX_ERR_NO_MEMORY;
    }

    proc->add_handle(vh);
    proc->add_handle(ph);

    /* Add new process to job */
    job->add_process(new_proc);

    /* Return handles */
    *proc_handle = proc->get_handle_user_val(ph);
    *vmar_handle = proc->get_handle_user_val(vh);
    return ZX_OK;
}

uint64_t sys_process_start(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 6);
    zx_handle_t proc_handle = seL4_GetMR(0);
    zx_handle_t thrd_handle = seL4_GetMR(1);
    uintptr_t entry = seL4_GetMR(2);
    uintptr_t stack = seL4_GetMR(3);
    zx_handle_t arg1 = seL4_GetMR(4);
    uintptr_t arg2 = seL4_GetMR(5);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    ZxProcess *target_proc;
    err = proc->get_object_with_rights(proc_handle, ZX_RIGHT_WRITE, target_proc);
    SYS_RET_IF_ERR(err);

    ZxThread *target_thrd;
    err = proc->get_object_with_rights(thrd_handle, ZX_RIGHT_WRITE, target_thrd);
    SYS_RET_IF_ERR(err);

    /* Check that the thread is owned by target proc */
    if ((ZxProcess *)target_thrd->get_owner() != target_proc) {
        return ZX_ERR_ACCESS_DENIED;
    }

    /* Check that target proc isn't already running or dead */
    if (target_proc->is_running() || target_proc->is_dead()) {
        return ZX_ERR_BAD_STATE;
    }

    /* Transfer arg1 handle to target proc */
    Handle *arg_handle = proc->get_handle(arg1);

    if (arg_handle == NULL) {
        return ZX_ERR_BAD_HANDLE;
    } else if (!arg_handle->has_rights(ZX_RIGHT_TRANSFER)) {
        return ZX_ERR_ACCESS_DENIED;
    }

    proc->remove_handle(arg_handle);

    zx_handle_t arg_uval;
    target_proc->add_handle(arg_handle);
    arg_uval = target_proc->get_handle_user_val(arg_handle);

    /* Start thread */
    if (target_thrd->start_execution(entry, stack, arg_uval, arg2) != 0) {
        /* Transfer the arg handle back to calling proc */
        target_proc->remove_handle(arg_handle);
        proc->add_handle(arg_handle);
        return ZX_ERR_BAD_STATE;
    }

    /* Notify proc of running thread */
    target_proc->thread_started();

    return ZX_OK;
}

uint64_t sys_process_exit(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    int retcode = seL4_GetMR(0);

    ZxProcess *proc = get_proc_from_badge(badge);
    proc->set_retcode(retcode);
    task_kill_process(proc);

    server_should_not_reply();
    return 0;
}

static uint64_t sys_process_rw_memory(seL4_MessageInfo_t tag, uint64_t badge,
        bool is_write)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t proc_handle = seL4_GetMR(0);
    uintptr_t vaddr = seL4_GetMR(1);
    uintptr_t user_buf = seL4_GetMR(2);
    size_t len = seL4_GetMR(3);
    uintptr_t user_actual = seL4_GetMR(4);

    if (len == 0 || len > SysTasks::kMaxDebugReadBlock) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    void *buf;
    size_t *actual;
    err = proc->uvaddr_to_kvaddr(user_buf, len, buf);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_actual, actual);
    SYS_RET_IF_ERR(err);

    /* Get the process we want to r/w from. */
    ZxProcess *target_proc;
    zx_rights_t req_rights = ZX_RIGHT_WRITE;

    if (!is_write) {
        req_rights |= ZX_RIGHT_READ;
    }

    err = proc->get_object_with_rights(proc_handle, req_rights, target_proc);
    SYS_RET_IF_ERR(err);

    /* Get the vmo mapping in proc */
    VmoMapping *vmap = target_proc->get_root_vmar()->get_vmap_from_addr(vaddr);

    if (vmap == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Get offset in vmo and actual len */
    uint64_t offset = vmap->addr_to_offset(vaddr);
    size_t max_len = vmap->get_size() - (vaddr - vmap->get_base());
    len = (len > max_len) ? max_len : len;

    /* Ensure backing pages are committed */
    ZxVmo *vmo = (ZxVmo *)vmap->get_owner();

    if (!vmo->commit_range(offset, len)) {
        return ZX_ERR_NO_MEMORY;
    }

    if (is_write) {
        vmo->write(offset, len, buf);
    } else {
        vmo->read(offset, len, buf);
    }

    *actual = len;
    return ZX_OK;
}

uint64_t sys_process_read_memory(seL4_MessageInfo_t tag, uint64_t badge)
{
    return sys_process_rw_memory(tag, badge, false);
}

uint64_t sys_process_write_memory(seL4_MessageInfo_t tag, uint64_t badge)
{
    return sys_process_rw_memory(tag, badge, true);
}

/* Thread syscalls */

uint64_t sys_thread_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t proc_handle = seL4_GetMR(0);
    uintptr_t user_buf = seL4_GetMR(1);
    uint32_t name_len = seL4_GetMR(2);
    uint32_t options = seL4_GetMR(3);
    uintptr_t user_out = seL4_GetMR(4);

    if (options != 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    void *name_buf;
    zx_handle_t *out;
    err = proc->uvaddr_to_kvaddr(user_buf, name_len, name_buf);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_out, out);
    SYS_RET_IF_ERR(err);

    /* Get the process we are adding the thread to */
    ZxProcess *target_proc;
    err = proc->get_object_with_rights(proc_handle, ZX_RIGHT_WRITE, target_proc);
    SYS_RET_IF_ERR(err);

    ZxThread *thrd;
    thrd = allocate_object<ZxThread>();

    if (thrd == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    thrd->set_name((char *)name_buf);

    /* Init thread, add to target proc */
    if (!thrd->init() || !target_proc->add_thread(thrd)) {
        destroy_object(thrd);
        return ZX_ERR_NO_MEMORY;
    }

    /* Create handle */
    zx_handle_t thrd_handle = proc->create_handle_get_uval(thrd);

    if (thrd_handle == ZX_HANDLE_INVALID) {
        destroy_object(thrd);
        return ZX_ERR_NO_MEMORY;
    }

    *out = thrd_handle;
    return ZX_OK;
}

uint64_t sys_thread_start(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t thrd_handle = seL4_GetMR(0);
    uintptr_t entry = seL4_GetMR(1);
    uintptr_t stack = seL4_GetMR(2);
    uintptr_t arg1 = seL4_GetMR(3);
    uintptr_t arg2 = seL4_GetMR(4);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    ZxThread *thrd;
    err = proc->get_object_with_rights(thrd_handle, ZX_RIGHT_WRITE, thrd);
    SYS_RET_IF_ERR(err);

    /* Make sure this thread isn't already alive or dead */
    if (thrd->is_alive() || thrd->is_dead()) {
        return ZX_ERR_BAD_STATE;
    }

    /* Make sure this isn't the first thread (use zx_process_start) */
    ZxProcess *owner_proc = (ZxProcess *)thrd->get_owner();

    if (!owner_proc->is_running()) {
        return ZX_ERR_BAD_STATE;
    }

    /* Start thread */
    if (thrd->start_execution(entry, stack, arg1, arg2) != 0) {
        return ZX_ERR_BAD_STATE;
    }

    /* Notify owner proc of running thread */
    owner_proc->thread_started();

    return ZX_OK;
}

uint64_t sys_thread_exit(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 0);

    ZxThread *thrd = get_thread_from_badge(badge);
    task_kill_thread(thrd);

    server_should_not_reply();
    return 0;
}

/* Task syscalls */

uint64_t sys_task_kill(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    zx_handle_t handle = seL4_GetMR(0);

    ZxProcess *proc = get_proc_from_badge(badge);

    Handle *h = proc->get_handle(handle);

    if (h == NULL) {
        return ZX_ERR_BAD_HANDLE;
    }

    if (!h->has_rights(ZX_RIGHT_DESTROY)) {
        return ZX_ERR_ACCESS_DENIED;
    }

    /* If this fails, object wasn't actually a task */
    if (!task_kill(h->get_object())) {
        return ZX_ERR_WRONG_TYPE;
    }

    /* If a proc/thread/job used this to kill itself, we
       can still reply, the invocation will just fail. */
    return ZX_OK;
}

uint64_t sys_task_suspend(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    zx_handle_t handle = seL4_GetMR(0);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    /* Only supports threads on native Zircon, so we do the same */
    ZxThread *target_thrd;
    err = proc->get_object_with_rights(handle, ZX_RIGHT_WRITE, target_thrd);
    SYS_RET_IF_ERR(err);

    if (!target_thrd->is_alive()) {
        return ZX_ERR_BAD_STATE;
    }

    target_thrd->suspend();
    return ZX_OK;
}

uint64_t sys_task_resume(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    zx_handle_t handle = seL4_GetMR(0);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    /* Only supports threads on native Zircon, so we do the same */
    ZxThread *target_thrd;
    err = proc->get_object_with_rights(handle, ZX_RIGHT_WRITE, target_thrd);
    SYS_RET_IF_ERR(err);

    if (!target_thrd->is_alive()) {
        return ZX_ERR_BAD_STATE;
    }

    target_thrd->resume();
    return ZX_OK;
}
