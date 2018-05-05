#include "object/thread.h"
#include "server.h"

namespace ThreadCxx {

constexpr size_t kMaxThreadCount = 4096u;

/* Thread table dimensions */
constexpr uint32_t kThreadIndexMask = kMaxThreadCount - 1;
constexpr size_t kThreadTableSize = sizeof(ZxThread) * kMaxThreadCount;
constexpr size_t kThreadTableNumPages = (kThreadTableSize + BIT(seL4_PageBits) - 1) / BIT(seL4_PageBits);

/* Thread cspace size. Should be as small as possible */
constexpr size_t kThreadCspaceBits = 3;
constexpr size_t kThreadBadgeShift = 12;

StackAlloc<ZxThread> thread_table;

void set_dest_slot(cspacepath_t *dest, seL4_CPtr root, seL4_CPtr slot)
{
    dest->root = root;
    dest->capPtr = slot;
    dest->capDepth = kThreadCspaceBits;
}

} /* namespace ThreadCxx */

/*
 *  ZxThread non-member functions
 */
void init_thread_table(vspace_t *vspace)
{
    using namespace ThreadCxx;

    /* Configure pages for thread pool */
    vspace_new_pages_config_t config;
    default_vspace_new_pages_config(kThreadTableNumPages, seL4_PageBits, &config);
    vspace_new_pages_config_set_vaddr((void *)ZX_THREAD_TABLE_START, &config);

    /* Allocate thread pool */
    void *thread_pool = vspace_new_pages_with_config(vspace, &config, seL4_AllRights);
    assert(thread_pool != NULL);

    /* Create alloc object */
    assert(thread_table.init(thread_pool, kMaxThreadCount));

    dprintf(ALWAYS, "Thread table created at %p, %lu pages\n", thread_pool, kThreadTableNumPages);
    dprintf(ALWAYS, "End of thread table at %p\n", (void *)(((uintptr_t)thread_pool)
            + (kThreadTableNumPages * BIT(seL4_PageBits))));
}

ZxThread *get_thread_from_badge(uint64_t badge)
{
    using namespace ThreadCxx;

    uint32_t index = (badge & kThreadIndexMask);
    return thread_table.get(index);
}

/* We use a different allocator for threads,
   so we require a template specialisation */
template <>
ZxThread *allocate_object<ZxThread>()
{
    using namespace ThreadCxx;

    uint32_t index;
    if (!thread_table.alloc(index)) {
        return NULL;
    }
    void *p = (void *)thread_table.get(index);
    return new (p) ZxThread(index);
}

template <>
void free_object<ZxThread>(ZxThread *obj)
{
    using namespace ThreadCxx;

    uint32_t index = obj->get_thread_index();
    thread_table.free(index);
}

/*
 *  ZxThread member functions
 */
bool ZxThread::init()
{
    using namespace ThreadCxx;

    int error;
    vka_t *vka = get_server_vka();
    cspacepath_t src;
    cspacepath_t dest = {0};

    /* If we fail at any point, destroy is capable of
       cleaning up a partially intialised thread */

    /* Create cspace */
    error = vka_alloc_cnode_object(vka, kThreadCspaceBits, &cspace_);
    if (error) {
        return false;
    }

    /* Get path to server ep */
    vka_cspace_make_path(vka, get_server_ep(), &src);

    /* Mint fault ep cap */
    set_dest_slot(&dest, cspace_.cptr, ZX_THREAD_FAULT_SLOT);
    error = vka_cnode_mint(&dest, &src, seL4_AllRights,
            seL4_CapData_Badge_new(ZxFaultBadge | thread_index_));
    if (error) {
        return false;
    }

    /* Mint syscall ep cap */
    set_dest_slot(&dest, cspace_.cptr, ZX_THREAD_SYSCALL_SLOT);
    error = vka_cnode_mint(&dest, &src, seL4_AllRights,
            seL4_CapData_Badge_new(ZxSyscallBadge | thread_index_));
    if (error) {
        return false;
    }

    /* Create TCB */
    error = vka_alloc_tcb(vka, &tcb_);
    if (error) {
        return false;
    }

    /* Allocate reply cap slot */
    error = vka_cspace_alloc(vka, &reply_cap_ );
    if (error) {
        reply_cap_ = 0;
        return false;
    }

    start_time_ = get_system_time();

    return true;
}

int ZxThread::configure_tcb(seL4_CNode pd, uintptr_t ipc_buffer_addr)
{
    using namespace ThreadCxx;

    seL4_CapData_t cspace_root_data = seL4_CapData_Guard_new(0, seL4_WordBits - kThreadCspaceBits);
    seL4_CapData_t null_cap_data = {{0}};
    return seL4_TCB_Configure(tcb_.cptr, ZX_THREAD_FAULT_SLOT, seL4_PrioProps_new(0,0), cspace_.cptr,
                        cspace_root_data, pd, null_cap_data, ipc_buffer_addr, ipc_buffer_frame_.cptr);
}

int ZxThread::start_execution(uintptr_t entry, uintptr_t stack,
        uintptr_t arg1, uintptr_t arg2)
{
#ifdef CONFIG_ARCH_X86_64
    seL4_UserContext context = {0};
    size_t context_size = sizeof(context) / sizeof(seL4_Word);

    context.rsp = stack;
    context.rip = entry;
    context.rdi = arg1;
    context.rsi = arg2;

    /* Note that we always resume the thread */
    return seL4_TCB_WriteRegisters(tcb_.cptr, 1, 0, context_size, &context);
#else
    return -1;
#endif
}

void ZxThread::destroy()
{
    using namespace ThreadCxx;

    vka_t *vka = get_server_vka();

    /* IPC buffer destroyed when thread was removed
       from proc. Destroy everything else. */

    /* Delete cspace */
    if (cspace_.cptr != 0) {
        vka_free_object(vka, &cspace_);
    }

    /* Delete tcb */
    if (tcb_.cptr != 0) {
        vka_free_object(vka, &tcb_);
    }

    /* Free reply cap slot */
    if (reply_cap_ != 0) {
        vka_cspace_free(vka, reply_cap_);
    }
}

void ZxThread::destroy_ipc_buffer()
{
    vka_t *vka = get_server_vka();
    seL4_ARCH_Page_Unmap(ipc_buffer_frame_.cptr);
    vka_free_object(vka, &ipc_buffer_frame_);
}

void ZxThread::wait(timer_callback_func cb, void *data,
        uint64_t expire_time, uint32_t flags)
{
    vka_t *vka = get_server_vka();
    cspacepath_t path;
    vka_cspace_make_path(vka, reply_cap_, &path);

    int error = vka_cnode_saveCaller(&path);
    if (error) {
        dprintf(CRITICAL, "Save caller returned %d\n", error);
    }

    /* Set callback & add timer */
    timer_.set_callback(cb, data);
    add_timer(&timer_, expire_time, flags);
}

zx_status_t ZxThread::obj_wait_one(Handle *h, zx_signals_t signals,
        zx_time_t deadline, zx_signals_t *observed)
{
    /* Allocate a waiter */
    StateWaiter *sw = allocate_object<StateWaiter>((ZxObject *)this);
    if (sw == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Assume a state match already checked */
    zx_signals_t initial_state = h->get_object()->get_signals();
    sw->init_wait(signals, initial_state, h);
    sw->set_data((void *)observed);

    wait(obj_wait_cb, (void *)this, deadline, 0);
    return ZX_OK;
}

void ZxThread::signal_observed(StateWaiter *sw)
{
    /* If num waiters set to zero, we are doing a wait one */
    if (num_waiting_on_ == 0) {
        /* Set the ret val */
        zx_signals_t *observed = (zx_signals_t *)sw->get_data();
        *observed = sw->get_observed();
        /* Clean up waiter */
        delete sw;
        waiting_on_ = NULL;
        /* Wake thread */
        resume_from_wait(ZX_OK);
        return;
    }

    /* Otherwise we have a wait many. */
    // TODO
}

/*
 * Timer callbacks
 */
void nanosleep_cb(void *data)
{
    ZxThread *thrd = (ZxThread *)data;
    thrd->resume_from_wait(ZX_OK);
}

void obj_wait_cb(void *data)
{
    ZxThread *thrd = (ZxThread *)data;
    StateWaiter *sw = (StateWaiter *)thrd->waiting_on_;
    if (thrd->num_waiting_on_ == 0) {
        /* We still set observed signals */
        zx_signals_t *observed = (zx_signals_t *)sw->get_data();
        *observed = sw->get_observed();
        delete sw;
        thrd->waiting_on_ = NULL;
        thrd->resume_from_wait(ZX_ERR_TIMED_OUT);
        return;
    }
    // TODO wait many
}
