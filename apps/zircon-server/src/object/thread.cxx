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

    return true;
}

int ZxThread::configure_tcb(seL4_CNode pd, uintptr_t ipc_buffer_addr)
{
    using namespace ThreadCxx;

    seL4_CapData_t cspace_root_data = seL4_CapData_Guard_new(0, seL4_WordBits - kThreadCspaceBits);
    seL4_CapData_t null_cap_data = {{0}};
    return seL4_TCB_Configure(tcb_.cptr, ZX_THREAD_FAULT_SLOT, seL4_PrioProps_new(0,0),
                            cspace_.cptr, cspace_root_data, pd, null_cap_data,
                            (seL4_Word)ipc_buffer_addr, ipc_buffer_frame_.cptr);
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

    /* TODO: if reply cap saved, free slot */
}

void ZxThread::destroy_ipc_buffer()
{
    vka_t *vka = get_server_vka();
    seL4_ARCH_Page_Unmap(ipc_buffer_frame_.cptr);
    vka_free_object(vka, &ipc_buffer_frame_);
}
