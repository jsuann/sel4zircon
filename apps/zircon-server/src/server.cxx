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

#include "utils/elf.h"
#include "utils/rng.h"

/* Wrap globals in a namespace to prevent access outside this file */
namespace ServerCxx {

/* seL4 stuff used by server */
vka_t *server_vka;
vspace_t *server_vspace;
seL4_CPtr server_ep;

} /* namespace ServerCxx */

extern "C" void do_cpp_test(void);
extern "C" void init_zircon_server(vka_t *vka, vspace_t *vspace, seL4_CPtr new_ep);
extern "C" void syscall_loop(void);

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

    /* init allocators and other things */
    init_handle_table(server_vspace);
    init_proc_table(server_vspace);
    init_vmo_kmap();
    init_prng();
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
        } else if (badge & ZxSyscallBadge) {
            seL4_Word syscall = seL4_MessageInfo_get_label(tag);
            if (syscall >= NUM_SYSCALLS) {
                /* syscall doesn't exist */
                sys_reply(ZX_ERR_BAD_SYSCALL);
            } else {
                DO_SYSCALL(syscall, tag, badge);
            }
        } else {
            dprintf(INFO, "Received non-zircon IPC!\n");
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
#include "syscalls/vmo.cxx"

#include "utils/elf.cxx"
#include "utils/init_test.cxx"

/* Extra test function */
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
