#include "object/thread.h"
#include "server.h"

namespace ThreadCxx {

/* Thread cspace size. Should be as small as possible */
constexpr size_t kThreadCspaceBits = 3;
constexpr size_t kThreadBadgeShift = 12;

void set_dest_slot(cspacepath_t *dest, seL4_CPtr root, seL4_CPtr slot)
{
    dest->root = root;
    dest->capPtr = slot;
    dest->capDepth = kThreadCspaceBits;
}

} /* namespace ThreadCxx */

/*
 *  ZxThread member functions
 */
int ZxThread::copy_cap_to_thread(cspacepath_t *src, seL4_CPtr slot)
{
    using namespace ThreadCxx;

    cspacepath_t dest = {0};
    set_dest_slot(&dest, cspace_.cptr, slot);
    return vka_cnode_copy(&dest, src, seL4_AllRights);
}

bool ZxThread::init()
{
    using namespace ThreadCxx;

    /* TODO proper error handling, cleanup */
    int error;
    vka_t *vka = get_server_vka();
    cspacepath_t src;
    cspacepath_t dest = {0};

    /* Create cspace */
    error = vka_alloc_cnode_object(vka, kThreadCspaceBits, &cspace_);
    assert(!error);

    /* Get path to server ep */
    vka_cspace_make_path(vka, get_server_ep(), &src);

    /* Create badge val */
    uint64_t badge_val = (thread_index_ << kThreadBadgeShift) | proc_index_;
    dprintf(INFO, "badge val: %lu\n", badge_val);

    /* Mint fault ep cap */
    set_dest_slot(&dest, cspace_.cptr, ZX_THREAD_FAULT_SLOT);
    error = vka_cnode_mint(&dest, &src, seL4_AllRights, seL4_CapData_Badge_new(ZxFaultBadge | badge_val));
    assert(!error);

    /* Mint syscall ep cap */
    set_dest_slot(&dest, cspace_.cptr, ZX_THREAD_SYSCALL_SLOT);
    error = vka_cnode_mint(&dest, &src, seL4_AllRights, seL4_CapData_Badge_new(ZxSyscallBadge | badge_val));
    assert(!error);

    /* Create TCB */
    error = vka_alloc_tcb(vka, &tcb_);
    assert(!error);

    return true;
}

int ZxThread::configure_tcb(seL4_CNode pd)
{
    using namespace ThreadCxx;

    seL4_CapData_t cspace_root_data = seL4_CapData_Guard_new(0, seL4_WordBits - kThreadCspaceBits);
    seL4_CapData_t null_cap_data = {{0}};
    return seL4_TCB_Configure(tcb_.cptr, ZX_THREAD_FAULT_SLOT, seL4_PrioProps_new(0,0),
                            cspace_.cptr, cspace_root_data, pd, null_cap_data,
                            (seL4_Word)ipc_buffer_addr_, ipc_buffer_frame_.cptr);
}

void ZxThread::destroy()
{
    using namespace ThreadCxx;
}
