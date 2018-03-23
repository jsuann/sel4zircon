#include "object/thread.h"
#include "server.h"

/* Thread cspace size. Should be as small as possible */
constexpr size_t kThreadCspaceBits = 4;
constexpr size_t kThreadBadgeShift = 12;

void set_dest_slot(cspacepath_t *dest, seL4_CPtr root, seL4_CPtr slot)
{
    dest->root = root;
    dest->capPtr = slot;
    dest->capDepth = kThreadCspaceBits;
}

/*
 *  ZxThread member functions
 */
int ZxThread::copy_cap_to_thread(cspacepath_t *src, seL4_CPtr slot)
{
    cspacepath_t dest = {0};
    set_dest_slot(&dest, cspace_.cptr, slot);
    return vka_cnode_copy(&dest, src, seL4_AllRights);
}

bool ZxThread::init()
{
    /* TODO proper error handling, cleanup */
    int error;
    vka_t *vka = get_server_vka();
    cspacepath_t src;
    cspacepath_t dest = {0};
    seL4_CapData_t cspace_root_data = seL4_CapData_Guard_new(0, seL4_WordBits - kThreadCspaceBits);

    /* Create cspace */
    error = vka_alloc_cnode_object(vka, kThreadCspaceBits, &cspace_);
    assert(!error);

    /* Mint CNode cap into new cspace */
    vka_cspace_make_path(vka, cspace_.cptr, &src);
    set_dest_slot(&dest, cspace_.cptr, SEL4UTILS_CNODE_SLOT);
    error = vka_cnode_mint(&dest, &src, seL4_AllRights, cspace_root_data);
    assert(!error);

    /* Get path to server ep */
    vka_cspace_make_path(vka, get_server_ep(), &src);

    /* Create badge val */
    uint64_t badge_val = (thread_index_ << kThreadBadgeShift) | proc_index_;

    /* Mint fault ep cap */
    set_dest_slot(&dest, cspace_.cptr, SEL4UTILS_ENDPOINT_SLOT);
    error = vka_cnode_mint(&dest, &src, seL4_AllRights, seL4_CapData_Badge_new(ZxFaultBadge & badge_val));
    assert(!error);

    /* Mint syscall ep cap */
    set_dest_slot(&dest, cspace_.cptr, SEL4UTILS_FIRST_FREE);
    error = vka_cnode_mint(&dest, &src, seL4_AllRights, seL4_CapData_Badge_new(badge_val));
    assert(!error);

    /* Create TCB */
    error = vka_alloc_tcb(vka, &tcb_);
    assert(!error);

    /* Copy TCB cap to cspace */
    vka_cspace_make_path(vka, tcb_.cptr, &src);
    set_dest_slot(&dest, cspace_.cptr, SEL4UTILS_TCB_SLOT);
    error = vka_cnode_copy(&dest, &src, seL4_AllRights);
    assert(!error);

    return true;
}

int ZxThread::configure_tcb(seL4_CNode pd)
{
    seL4_CapData_t cspace_root_data = seL4_CapData_Guard_new(0, seL4_WordBits - kThreadCspaceBits);
    seL4_CapData_t null_cap_data = {{0}};
    return seL4_TCB_Configure(tcb_.cptr, get_server_ep(), seL4_PrioProps_new(0,0),
                            cspace_.cptr, cspace_root_data, pd, null_cap_data,
                            (seL4_Word)ipc_buffer_addr_, ipc_buffer_frame_.cptr);
}

void ZxThread::destroy()
{
}
