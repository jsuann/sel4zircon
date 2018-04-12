#include "object/process.h"
#include "zxcpp/stackalloc.h"
#include "server.h"

extern "C" {
#include <sel4utils/process.h>
}

namespace ProcessCxx {

/* If 1024 or greater, we need to create more ASID pools */
constexpr size_t kMaxProcCount = 512u;

constexpr uint32_t kProcIndexMask = kMaxProcCount - 1;
constexpr size_t kProcTableSize = sizeof(ZxProcess) * kMaxProcCount;
constexpr size_t kProcTableNumPages = (kProcTableSize + BIT(seL4_PageBits) - 1) / BIT(seL4_PageBits);

StackAlloc<ZxProcess> proc_table;

/* Limit for threads per process */
constexpr size_t kMaxThreadPerProc = 256u;
constexpr size_t kProcThreadAllocSize = kMaxThreadPerProc / 8;

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

} /* namespace ProcessCxx */

/*
 *  ZxProcess non-member functions
 */
void init_proc_table(vspace_t *vspace)
{
    using namespace ProcessCxx;

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
    using namespace ProcessCxx;

    uint32_t index = (badge & kProcIndexMask);
    return proc_table.get(index);
}

template <>
ZxProcess *allocate_object<ZxProcess>(ZxVmar *root_vmar)
{
    using namespace ProcessCxx;

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
    using namespace ProcessCxx;

    uint32_t index = obj->get_proc_index();
    proc_table.free(index);
}

/*
 *  ZxProcess member functions
 */
bool ZxProcess::init()
{
    using namespace ProcessCxx;

    int error;
    vka_t *vka = get_server_vka();

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

    return true;
}

void ZxProcess::destroy()
{
    using namespace ProcessCxx;

    thrd_alloc_.destroy();
}

bool ZxProcess::add_thread(ZxThread *thrd)
{
    using namespace ProcessCxx;

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

    /* Create IPC buffer frame */
    vka_object_t ipc_frame;
    error = vka_alloc_frame(vka, seL4_PageBits, &ipc_frame);
    assert(!error);

    /* Map IPC frame into vspace */
    error = map_page_in_vspace(ipc_frame.cptr, (void *)ipc_buf_addr, seL4_AllRights, 1);
    assert(!error);

    /* Assign IPC buffer to thread */
    thrd->set_ipc_buffer(ipc_frame, ipc_buf_addr);

    /* Configure TCB */
    dprintf(SPEW, "Conf tcb\n");
    error = thrd->configure_tcb(pd_.cptr);
    assert(!error);

    /* Add thread to list */
    thread_list_.push_back(thrd);
    return true;
}

int ZxProcess::map_page_in_vspace(seL4_CPtr frame_cap,
        void *vaddr, seL4_CapRights_t rights, int cacheable)
{
    int error;
    vka_object_t objects[3];
    int num_obj = 3;
    vka_t *vka = get_server_vka();

    /* Attempt page mapping */
    error = sel4utils_map_page(vka, pd_.cptr, frame_cap, vaddr,
            rights, cacheable, objects, &num_obj);
    if (error) {
        return error;
    }

    /* Store allocated PT objects */
    VkaObjectNode *head = pt_list_;
    int num_alloc = 0;
    for (int i = 0; i < num_obj; ++i) {
        head = new_vka_node(head, objects[i]);
        if (head == NULL) {
            break;
        }
        ++num_alloc;
    }

    /* If nodes successfully allocated, return */
    if (num_alloc == num_obj) {
        pt_list_ = head;
        return 0;
    }

    /* Otherwise we need to clean up */
    /* Unmap the page */
    seL4_X86_Page_Unmap(frame_cap);

    /* Free nodes & PT objects */
    VkaObjectNode *curr = head;
    for (int i = 0; i < num_alloc; ++i) {
        head = curr;
        curr = curr->next;
        vka_free_object(vka, &head->obj);
        free(head);
    }
    assert(curr == pt_list_);

    return -1;
}

/* Get server vaddr from user vaddr. Perform length check in process */
void *ZxProcess::uvaddr_to_kvaddr(uintptr_t uvaddr, size_t len, zx_status_t *err)
{
    *err = ZX_OK;

    /* Get the VMO mapping of uvaddr */
    VmoMapping *vmap = root_vmar_->get_vmap_from_addr(uvaddr);
    if (vmap == NULL) {
        *err = ZX_ERR_INVALID_ARGS;
        return NULL;
    }

    /* Ensure that addr + len won't exceed vmo end */
    if (uvaddr + len > vmap->get_base() + vmap->get_size()) {
        *err = ZX_ERR_INVALID_ARGS;
        return NULL;
    }

    /* Ensure addr is backed by a page */
    ZxVmo *vmo = (ZxVmo *)vmap->get_owner();
    uint64_t offset = uvaddr - vmap->get_base();
    if (!vmo->commit_page(offset / (1 << seL4_PageBits), vmap)) {
        *err = ZX_ERR_NO_MEMORY;
        return NULL;
    }

    /* Return kvaddr */
    uintptr_t kvaddr = vmo->get_base() + offset;
    return (void *)kvaddr;
}
