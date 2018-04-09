#include <autoconf.h>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "server.h"
#include "addrspace.h"
#include "zxcpp/new.h"

extern "C" {
#include <vka/object.h>
#include <zircon/types.h>
#include <zircon/rights.h>
#include <vspace/vspace.h>
#include "debug.h"
}

#include "syscalls.h"
#include "sys_helpers.h"

#include "object/handle.h"
#include "object/process.h"
#include "object/vmar.h"

#include "test/elf.h"

extern "C" void do_cpp_test(void);
extern "C" void init_zircon_server(vka_t *vka, vspace_t *vspace, seL4_CPtr new_ep);
extern "C" uint64_t init_zircon_test(void);
extern "C" void send_zircon_test_data(seL4_CPtr ep_cap);
extern "C" void syscall_loop(void);

/* Wrap globals in a namespace to prevent access outside this file */
namespace ServerCxx {

/* seL4 stuff used by server */
vka_t *server_vka;
vspace_t *server_vspace;
seL4_CPtr server_ep;
/* Base zx objects for zircon test */
ZxVmar *test_vmar;
ZxProcess *test_proc;
ZxThread *test_thread;
ZxVmo **elf_vmos;
ZxVmo *stack_vmo;

} /* namespace ServerCxx */

vspace_t *get_server_vspace()
{
    return ServerCxx::server_vspace;
}

vka_t *get_server_vka()
{
    return ServerCxx::server_vka;
}

seL4_CPtr get_server_ep()
{
    return ServerCxx::server_ep;
}

void init_zircon_server(vka_t *vka, vspace_t *vspace, seL4_CPtr new_ep)
{
    using namespace ServerCxx;

    /* store server globals */
    server_vka = vka;
    server_vspace = vspace;
    server_ep = new_ep;

    /* init allocators */
    init_handle_table(server_vspace);
    init_proc_table(server_vspace);
    init_vmo_kmap();
}

uint64_t init_zircon_test(void)
{
    using namespace ServerCxx;

    /* Create a root vmar */
    test_vmar = allocate_object<ZxVmar>();
    assert(test_vmar != NULL);

    /* Create a process */
    test_proc = allocate_object<ZxProcess>(test_vmar);
    assert(test_proc != NULL);
    test_proc->set_name("zircon-test");
    assert(test_proc->init());

    /* Create a thread */
    uint32_t thrd_index;
    uint32_t proc_index = test_proc->get_proc_index();
    assert(test_proc->alloc_thread_index(thrd_index));
    test_thread = allocate_object<ZxThread>(proc_index, thrd_index);
    assert(test_thread != NULL);
    test_thread->set_name("zircon-test-thread");
    assert(test_thread->init());
    test_proc->add_thread(test_thread);

    /* Create VMOs for elf segments */
    int num_vmos;
    uintptr_t entry = load_elf_segments(test_proc, "zircon-test", num_vmos, elf_vmos);
    assert(entry != 0);
    assert(elf_vmos != NULL);
    for (int i = 0; i < num_vmos; ++i) {
        assert(elf_vmos[i] != NULL);
    }

    /* Create stack VMO TODO: define constants */
    stack_vmo = allocate_object<ZxVmo>(15);
    assert(stack_vmo->init());
    VmoMapping *stack_map = stack_vmo->create_mapping(0x30000000, test_vmar,
            ZX_VM_FLAG_PERM_READ | ZX_VM_FLAG_PERM_WRITE);
    assert(stack_map != NULL);
    stack_vmo->commit_all_pages(stack_map);

    /* Return badge value for process */
    return test_proc->get_proc_index();
}

void send_zircon_test_data(seL4_CPtr ep_cap)
{
    using namespace ServerCxx;

    /* Get handles to test objects */
    Handle *vmar_handle = test_vmar->create_handle(ZX_DEFAULT_VMAR_RIGHTS);
    Handle *proc_handle = test_proc->create_handle(ZX_DEFAULT_PROCESS_RIGHTS);
    Handle *thrd_handle = test_thread->create_handle(ZX_DEFAULT_THREAD_RIGHTS);

    /* Add handles to process */
    test_proc->add_handle(vmar_handle);
    test_proc->add_handle(proc_handle);
    test_proc->add_handle(thrd_handle);
    test_proc->print_handles();

    /* Get user handle values */
    zx_handle_t vmar_uval = test_proc->get_handle_user_val(vmar_handle);
    zx_handle_t proc_uval = test_proc->get_handle_user_val(proc_handle);
    zx_handle_t thrd_uval = test_proc->get_handle_user_val(thrd_handle);

    /* Send handles to zircon test */
    dprintf(SPEW, "Sending test data to zircon test!\n");
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 3);
    seL4_SetMR(0, vmar_uval);
    seL4_SetMR(1, proc_uval);
    seL4_SetMR(2, thrd_uval);
    seL4_Send(ep_cap, tag);
}

void do_cpp_test(void)
{
    using namespace ServerCxx;

    dprintf(SPEW, "Root vmar base: %lx, size: %lx, end: %lx\n", ZX_USER_ASPACE_BASE, ZX_USER_ASPACE_SIZE,
            (ZX_USER_ASPACE_BASE+ZX_USER_ASPACE_SIZE));

    dprintf(SPEW, "Size of IPC buffer: %lu, size of message info %lu\n",
            sizeof(seL4_IPCBuffer), sizeof(seL4_MessageInfo_t));

    vka_object_t test_vka = {0};
    test_vka.size_bits = 12;
    test_vka = {0};
    assert(test_vka.size_bits == 0);
/*
    ZxVmar *vmar1 = allocate_object<ZxVmar>();
    assert(vmar1 != NULL);
    ZxProcess *p1 = allocate_object<ZxProcess>(vmar1);
    assert(p1 != NULL);

    Handle *h1 = vmar1->create_handle(ZX_RIGHT_READ);
    assert(h1 != NULL);
    Handle *h2 = p1->create_handle(ZX_RIGHT_READ);
    assert(h2 != NULL);

    p1->add_handle(h1);
    p1->add_handle(h2);

    ZxObject *o1 = p1;
    dprintf(SPEW, "Type of p1: %u, should be: %u\n", p1->get_object_type(), ZX_OBJ_TYPE_PROCESS);
    dprintf(SPEW, "Type of o1: %u, should be: %u\n", o1->get_object_type(), ZX_OBJ_TYPE_PROCESS);

    p1->print_object_info();
    vmar1->print_object_info();

    dprintf(SPEW, "Handles:\n");
    p1->print_handles();

    // Object destroyal: destroy handle, destroy object if handle count == 0
    p1->remove_handle(h1);
    dprintf(SPEW, "Remove vmar handle!\n");
    if (vmar1->destroy_handle(h1)) {
        dprintf(SPEW, "Destroy vmar!\n");
        free_object(vmar1);
    }

    p1->print_handles();

    p1->remove_handle(h2);
    if (p1->destroy_handle(h2)) {
        dprintf(SPEW, "Destroy process!\n");
        free_object(p1);
    }
*/
}

void syscall_loop(void)
{
    using namespace ServerCxx;

    seL4_Word badge = 0;
    seL4_MessageInfo_t tag;

    dprintf(INFO, "Entering syscall loop\n");
    for (;;) {
        tag = seL4_Recv(server_ep, &badge);
        /* Check for fault, irq, syscall */
        if (badge & ZxFaultBadge) {
            seL4_Word fault_type = seL4_MessageInfo_get_label(tag);
            dprintf(INFO, "Received fault, type %lu\n", fault_type);
        } else {
            seL4_Word syscall = seL4_MessageInfo_get_label(tag);
            if (syscall >= NUM_SYSCALLS) {
                /* syscall doesn't exist */
                sys_reply(ZX_ERR_BAD_SYSCALL);
            } else {
                DO_SYSCALL(syscall, tag, badge);
            }
        }
    }
}

/*
 * We need to include other cxx files in subdirs, rather than compile
 * them separately. This is due to enums defined in sel4/sel4.h causing
 * linker issues. Namespaces are used to provided inter-file safety.
 */
#include "object/handle.cxx"
#include "object/object.cxx"
#include "object/process.cxx"
#include "object/thread.cxx"
#include "object/vmar.cxx"
#include "object/vmo.cxx"

#include "syscalls/sys_table.cxx"
#include "syscalls/channel.cxx"
#include "syscalls/handle.cxx"
#include "syscalls/other.cxx"
#include "syscalls/tasks.cxx"
#include "syscalls/tests.cxx"

#include "test/elf.cxx"
