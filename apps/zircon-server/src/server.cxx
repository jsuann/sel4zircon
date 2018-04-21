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

#include "object/job.h"
#include "object/handle.h"
#include "object/vmar.h"
#include "object/mbuf.h"

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

    dprintf(ALWAYS, "=== Zircon Server ===\n");

    /* store server globals */
    server_vka = vka;
    server_vspace = vspace;
    server_ep = new_ep;

    /* init allocators and other things */
    init_handle_table(server_vspace);
    init_proc_table(server_vspace);
    init_vmo_kmap();
    init_prng();
    init_root_job();
    init_asid_pool(server_vka);
    init_page_buf(server_vka);
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
#include "object/job.cxx"
#include "object/process.cxx"
#include "object/thread.cxx"
#include "object/vmar.cxx"
#include "object/vmo.cxx"
#include "object/fifo.cxx"
#include "object/mbuf.cxx"
#include "object/channel.cxx"

#include "syscalls/sys_table.cxx"
#include "syscalls/channel.cxx"
#include "syscalls/fifo.cxx"
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

    /* Test mbuf */
    MBuf buf;

    size_t len = 30;
    char test[len] = {0};
    char str[] = "HELLO HELLO HELLO";

    assert(buf.write((uint8_t*)&str[0], strlen(str) + 1) == ZX_OK);

    dprintf(SPEW, "mbuf size: %lu\n", buf.get_size());

    assert(buf.read((uint8_t*)&test[0], len/2) == ZX_OK);
    dprintf(SPEW, "mbuf size: %lu\n", buf.get_size());
    assert(buf.read((uint8_t*)&test[len/2], len/2) == ZX_OK);

    dprintf(SPEW, "str: %s\n", test);
}
