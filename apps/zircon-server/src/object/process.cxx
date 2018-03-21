#include "object/process.h"
#include "zxcpp/stackalloc.h"
#include "server.h"

/* If 1024 or greater, we need to create more ASID pools */
constexpr size_t kMaxProcCount = 512u;

constexpr uint32_t kProcIndexMask = kMaxProcCount - 1;
constexpr size_t kProcTableSize = sizeof(ZxProcess) * kMaxProcCount;
constexpr size_t kProcTableNumPages = (kProcTableSize + BIT(seL4_PageBits) - 1) / BIT(seL4_PageBits);

StackAlloc<ZxProcess> proc_table;

/* Limit for threads per process */
constexpr size_t kMaxThreadPerProc = 256u;
constexpr size_t kProcThreadAllocSize = kMaxThreadPerProc / 8;

/*
 *  ZxProcess non-member functions
 */
void init_proc_table(vspace_t *vspace)
{
    /* Configure pages for proc pool */
    vspace_new_pages_config_t config;
    default_vspace_new_pages_config(kProcTableNumPages, seL4_PageBits, &config);
    vspace_new_pages_config_set_vaddr((void *)ZX_PROCESS_TABLE_START, &config);

    /* Allocate proc pool */
    void *proc_pool = vspace_new_pages_with_config(vspace, &config, seL4_AllRights);
    assert(proc_pool != NULL);
    memset(proc_pool, 0, kProcTableSize);

    /* Create alloc object */
    assert(proc_table.init(proc_pool, kMaxProcCount));

    dprintf(ALWAYS, "Proc table created at %p, %lu pages\n", proc_pool, kProcTableNumPages);
    dprintf(ALWAYS, "End of proc table at %p\n", (void *)(((uintptr_t)proc_pool)
            + (kProcTableNumPages * BIT(seL4_PageBits))));
}

ZxProcess *get_proc_from_badge(uint64_t badge)
{
    uint32_t index = (badge & kProcIndexMask);
    return proc_table.get(index);
}

template <>
ZxProcess *allocate_object<ZxProcess>(ZxVmar *root_vmar)
{
    uint32_t index;
    if (!proc_table.alloc(index)) {
        return NULL;
    }
    void *p = (void *)proc_table.get(index);
    return new (p) ZxProcess(root_vmar, index);
}

template <>
void free_object<ZxProcess>(ZxProcess *obj)
{
    uint32_t index = obj->get_proc_index();
    proc_table.free(index);
}

int assign_asid_pool(seL4_CPtr pd, seL4_CPtr *ret_pool)
{
    /* XXX just use init asid pool for now */
    int error = seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool, pd);
    if (error) {
        dprintf(CRITICAL, "Failed to assign asid pool to process!\n");
    }
    *ret_pool = seL4_CapInitThreadASIDPool;
    return error;
}

/*
 *  ZxProcess member functions
 */
bool ZxProcess::init()
{
    /* TODO proper error handling, cleanup */
    int error;
    vka_t *vka = get_server_vka();
    vspace_t *server_vspace = get_server_vspace();

    /* Create thread id allocator */
    if (!thrd_alloc_.init(kProcThreadAllocSize)) {
        return false;
    }

    /* Create PD */
    error = vka_alloc_vspace_root(vka, &pd_);
    assert(!error);

    /* Assign ASID pool */
    error = assign_asid_pool(pd_.cptr, &asid_pool_);
    assert(!error);

    /* Create a vspace */
    /* TODO: add allocated object fn */
    error = sel4utils_get_vspace(server_vspace, &vspace_, &data_, vka, pd_.cptr, NULL, NULL);
    assert(!error);

    return true;
}

bool ZxProcess::add_thread(ZxThread *thrd)
{
    /* TODO proper error handling, cleanup */
    int error;
    cspacepath_t src;
    vka_t *vka = get_server_vka();

    /* Copy PD cap into thread cspace */
    vka_cspace_make_path(vka, pd_.cptr, &src);
    error = thrd->copy_cap_to_thread(&src, SEL4UTILS_PD_SLOT);
    assert(!error);

    /* Get address of IPC buffer */
    uint32_t thrd_index = thrd->get_thread_index();
    uintptr_t ipc_buf_addr = ZX_USER_IPC_BUFFER_BASE + (BIT(seL4_PageBits) * thrd_index * 2);

    /* Create IPC buffer */
    vspace_new_pages_config_t config;
    default_vspace_new_pages_config(1, seL4_PageBits, &config);
    vspace_new_pages_config_set_vaddr((void *)ipc_buf_addr, &config);
    void *ipc_buf = vspace_new_pages_with_config(&vspace_, &config, seL4_AllRights);
    assert(ipc_buf != NULL);

    /* Get cap to IPC buffer */
    seL4_CPtr ipc_buf_cap = vspace_get_cap(&vspace_, ipc_buf);

    /* Assign IPC buffer to thread */
    thrd->set_ipc_buffer(ipc_buf_cap, ipc_buf);

    /* Configure TCB */
    error = thrd->configure_tcb(pd_.cptr);
    assert(!error);

    /* Add thread to list */
    thread_list_.push_back(thrd);
    return true;
}

void ZxProcess::destroy()
{
    thrd_alloc_.destroy();
}
