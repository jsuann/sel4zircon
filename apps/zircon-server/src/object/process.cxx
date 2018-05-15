#include "object/process.h"
#include "zxcpp/stackalloc.h"
#include "server.h"

namespace ProcessCxx {

/* If greater than 1024, we need to create more ASID pools */
constexpr size_t kMaxProcCount = 1024u;

constexpr size_t kProcTableSize = sizeof(ZxProcess) * kMaxProcCount;
constexpr size_t kProcTableNumPages = (kProcTableSize + BIT(seL4_PageBits) - 1) / BIT(seL4_PageBits);

StackAlloc<ZxProcess> proc_table;

/* If proc count > 1024, make this an array! */
seL4_CPtr proc_asid_pool;

/* Limit for threads per process */
constexpr size_t kMaxThreadPerProc = 256u;
constexpr size_t kIpcIndexAllocSize = kMaxThreadPerProc / 8;

int assign_asid_pool(seL4_CPtr pd, seL4_CPtr *ret_pool)
{
    int error = seL4_ARCH_ASIDPool_Assign(proc_asid_pool, pd);
    if (error) {
        dprintf(CRITICAL, "Failed to assign asid pool to process!\n");
    }
    *ret_pool = proc_asid_pool;
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

    /* Create alloc object */
    assert(proc_table.init(proc_pool, kMaxProcCount));

    dprintf(ALWAYS, "Proc table created at %p, %lu pages\n", proc_pool, kProcTableNumPages);
    dprintf(ALWAYS, "End of proc table at %p\n", (void *)(((uintptr_t)proc_pool)
            + (kProcTableNumPages * BIT(seL4_PageBits))));
}

void init_asid_pool(vka_t *vka)
{
    using namespace ProcessCxx;

    vka_object_t pool_ut;
    cspacepath_t path;

    assert(vka_alloc_untyped(vka, 12, &pool_ut) == 0);
    assert(vka_cspace_alloc_path(vka, &path) == 0);
    assert(seL4_ARCH_ASIDControl_MakePool(seL4_CapASIDControl, pool_ut.cptr,
                path.root, path.capPtr, path.capDepth) == 0);
    proc_asid_pool = path.capPtr;
}

ZxProcess *get_proc_from_badge(uint64_t badge)
{
    using namespace ProcessCxx;

    ZxThread *thrd = get_thread_from_badge(badge);
    assert(thrd->get_owner() != NULL); // XXX sanity check
    return (ZxProcess *)thrd->get_owner();
}

/* We use a different allocator for processes,
   so we require a template specialisation */
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

    /* If we fail at any point, destroy is capable of
       cleaning up a partially intialised proc */

    /* Create ipc index allocator */
    if (!ipc_alloc_.init(kIpcIndexAllocSize)) {
        return false;
    }

    /* Create PD */
    error = vka_alloc_vspace_root(vka, &pd_);
    if (error) {
        return false;
    }

    /* Assign ASID pool */
    error = assign_asid_pool(pd_.cptr, &asid_pool_);
    if (error) {
        return false;
    }

    return true;
}

void ZxProcess::destroy()
{
    using namespace ProcessCxx;

    vka_t *vka = get_server_vka();

    /* Threads should already be removed */
    assert(thread_list_.empty());

    /* Handles should also be destroyed */
    assert(handle_list_.empty());

    /* Deactivate the root VMAR and its subregions */
    deactivate_maybe_destroy_vmar(root_vmar_);

    /* Destroy thread index allocator */
    ipc_alloc_.destroy();

    /* Delete vka PT nodes */
    free_vka_nodes(vka, pt_list_);

    /* Destroy pd */
    if (pd_.cptr != 0) {
        vka_free_object(vka, &pd_);
    }

    /* Detach from the owning job */
    ZxJob *parent = (ZxJob *)get_owner();
    parent->remove_process(this);

    /* Parent job might have to be cleaned up */
    if (parent->can_destroy()) {
        return destroy_object(parent);
    }
}

bool ZxProcess::add_thread(ZxThread *thrd)
{
    using namespace ProcessCxx;

    int error;
    vka_t *vka = get_server_vka();

    vka_object_t ipc_frame = {0};

    /* Get index for ipc buf */
    uint32_t ipc_index;
    if (!ipc_alloc_.alloc(ipc_index)) {
        return false;
    }

    /* Calc address of IPC buffer */
    uintptr_t ipc_buf_addr = ZX_USER_IPC_BUFFER_BASE + (BIT(seL4_PageBits) * ipc_index * 2);

    /* Create IPC buffer frame */
    error = vka_alloc_frame(vka, seL4_PageBits, &ipc_frame);
    if (error) {
        ipc_alloc_.free(ipc_index);
        return false;
    }

    /* Map IPC frame into vspace */
    error = map_page_in_vspace(ipc_frame.cptr, (void *)ipc_buf_addr, seL4_AllRights, 1);
    if (error) {
        goto error_add_thread;
    }

    /* Assign IPC buffer to thread */
    thrd->set_ipc_buffer(ipc_frame, ipc_index);

    /* Configure TCB */
    error = thrd->configure_tcb(pd_.cptr, ipc_buf_addr);
    if (error) {
        seL4_ARCH_Page_Unmap(ipc_frame.cptr);
        goto error_add_thread;
    }

    /* Add thread to list */
    thread_list_.push_back(thrd);
    return true;

error_add_thread:
    ipc_alloc_.free(ipc_index);
    vka_free_object(vka, &ipc_frame);
    return false;
}

void ZxProcess::remove_thread(ZxThread *thrd)
{
    using namespace ProcessCxx;

    /* Destroy IPC buffer & free ipc index */
    uint32_t index = thrd->get_ipc_index();
    thrd->destroy_ipc_buffer();
    ipc_alloc_.free(index);

    /* Remove thread from list */
    thread_list_.remove(thrd);
}

void ZxProcess::kill()
{
    /* If already killed, return */
    if (state_ == State::DEAD) {
        return;
    }

    /* Put in killing state. This prevents children
       from destroying us */
    state_ = State::KILLING;

    /* Kill all threads */
    thread_list_.for_each([] (ZxThread *thrd) {
        thrd->kill();
        if (thrd->can_destroy()) {
            /* It's safe for thrd to remove itself
               from linked list in for_each */
            destroy_object(thrd);
        }
    });
    alive_count_ = 0;

    /* Close all handles held by the process */
    while (!handle_list_.empty()) {
        Handle *h = handle_list_.pop_front();
        destroy_handle_maybe_object(h);
    }

    /* Set state to dead */
    update_state(0u, ZX_TASK_TERMINATED);
    state_ = State::DEAD;
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
    seL4_ARCH_Page_Unmap(frame_cap);

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

void ZxProcess::remap_page_in_vspace(seL4_CPtr frame_cap,
        seL4_CapRights_t rights, int cacheable)
{
    seL4_ARCH_VMAttributes attr;
    attr = (cacheable) ? seL4_ARCH_Default_VMAttributes : seL4_ARCH_Uncached_VMAttributes;

    /* Ignore any errors */
    seL4_ARCH_Page_Remap(frame_cap, pd_.cptr, rights, attr);
}

/* Get server vaddr from user vaddr. Perform length check in process */
zx_status_t ZxProcess::uvaddr_to_kvaddr(uintptr_t uvaddr,
        size_t len, void *&kvaddr)
{
    kvaddr = NULL;

    /* Get the VMO mapping of uvaddr */
    VmoMapping *vmap = root_vmar_->get_vmap_from_addr(uvaddr);
    if (vmap == NULL) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Ensure that addr + len won't exceed vmo end */
    if (uvaddr + len > vmap->get_base() + vmap->get_size()) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Ensure addr is backed by a page */
    ZxVmo *vmo = (ZxVmo *)vmap->get_owner();
    uint64_t offset = uvaddr - vmap->get_base();
    if (!vmo->commit_page(offset / (1 << seL4_PageBits), vmap)) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Return kvaddr */
    kvaddr = (void *)(vmo->get_base() + offset);
    return ZX_OK;
}

template <typename T>
zx_status_t ZxProcess::get_object_with_rights(zx_handle_t handle_val,
        zx_rights_t rights, T *&obj)
{
    obj = NULL;

    Handle *h = get_handle(handle_val);
    if (h == NULL) {
        return ZX_ERR_BAD_HANDLE;
    }

    ZxObject *base = h->get_object();
    if (!is_object_type<T>(base)) {
        return ZX_ERR_WRONG_TYPE;
    }

    if (!h->has_rights(rights)) {
        return ZX_ERR_ACCESS_DENIED;
    }

    obj = (T *)base;
    return ZX_OK;
}
