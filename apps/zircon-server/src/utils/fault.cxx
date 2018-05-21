#include "utils/fault.h"
#include "object/tasks.h"

void handle_fault(seL4_MessageInfo_t tag, uint64_t badge)
{
    /* Get the faulting thread */
    ZxThread *thrd = get_thread_from_badge(badge);

    /* If we have a VM fault, try to map a page */
    if (seL4_isVMFault_tag(tag)) {
        uintptr_t vaddr = seL4_Fault_VMFault_get_Addr(seL4_getFault(tag));
        dprintf(SPEW, "Fault at vaddr 0x%lx\n", vaddr);
        /* Get the root vmar */
        ZxVmar *root_vmar = get_proc_from_badge(badge)->get_root_vmar();
        /* Find the vmo mapping for the faulting addr */
        VmoMapping *vmap = root_vmar->get_vmap_from_addr(vaddr);
        if (vmap != NULL) {
            if (vmap->commit_page_at_addr(vaddr)) {
                /* Page mapped in, we can restart thread & return */
                tag = seL4_MessageInfo_new(0, 0, 0, 0);
                seL4_Reply(tag);
                return;
            }
        }
    }

    /* TODO once exception port is implemented, a packet should
       be delivered to the thread's (or parent proc/job's) exception
       port. For now, we just kill the thread. */
    dprintf(INFO, "Killing faulting thread at %p\n", thrd);
    task_kill_thread(thrd);
}
