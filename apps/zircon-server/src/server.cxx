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
#include <sel4platsupport/timer.h>
#include "debug.h"
}

#include "syscalls.h"
#include "sys_helpers.h"

#include "object/job.h"
#include "object/handle.h"
#include "object/vmar.h"
#include "object/mbuf.h"

#include "utils/clock.h"
#include "utils/elf.h"
#include "utils/rng.h"
#include "utils/page_alloc.h"

/* Wrap globals in a namespace to prevent access outside this file */
namespace ServerCxx {

/* seL4 stuff used by server */
vka_t *server_vka;
vspace_t *server_vspace;
seL4_CPtr server_ep;
seL4_Word timer_badge;

bool should_reply;

} /* namespace ServerCxx */

extern "C" void do_cpp_test(void);
extern "C" void init_zircon_server(vka_t *vka, vspace_t *vspace,
        seL4_timer_t *timer, seL4_CPtr new_ep, seL4_CPtr ntfn);
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

void server_should_not_reply()
{
    ServerCxx::should_reply = false;
}

void init_zircon_server(vka_t *vka, vspace_t *vspace,
        seL4_timer_t *timer, seL4_CPtr new_ep, seL4_CPtr ntfn)
{
    using namespace ServerCxx;

    dprintf(ALWAYS, "=== Zircon Server ===\n");

    /* store server globals */
    server_vka = vka;
    server_vspace = vspace;
    server_ep = new_ep;

    /* Init timer */
    init_timer(timer, ntfn, seL4_CapInitThreadTCB, &timer_badge);

    /* init allocators and other things */
    init_handle_table(server_vspace);
    init_proc_table(server_vspace);
    init_thread_table(server_vspace);
    init_vmo_kmap();
    init_prng();
    init_root_job();
    init_asid_pool(server_vka);
    init_page_alloc(server_vka);
}

void syscall_loop(void)
{
    using namespace ServerCxx;

    uint64_t badge, ret;
    seL4_MessageInfo_t tag;

    dprintf(INFO, "Entering syscall loop!\n");

    for (;;) {
        should_reply = true;
        tag = seL4_Recv(server_ep, &badge);
        if (badge & ZxSyscallBadge) {
            seL4_Word syscall = seL4_MessageInfo_get_label(tag);
            if (unlikely(syscall >= NUM_SYSCALLS)) {
                /* syscall doesn't exist */
                sys_reply(ZX_ERR_BAD_SYSCALL);
            } else {
                ret = DO_SYSCALL(syscall, tag, badge);
                /* nearly all syscalls reply immediately */
                if (likely(should_reply)) {
                    sys_reply(ret);
                }
            }
        } else if (badge & ZxFaultBadge) {
            seL4_Word fault_type = seL4_MessageInfo_get_label(tag);
            dprintf(INFO, "Received fault, type %lu\n", fault_type);
        } else if (badge == timer_badge) {
            /* Handle timer interrupt */
            handle_timer(badge);
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
#include "object/socket.cxx"
#include "object/waiter.cxx"

#include "syscalls/sys_table.cxx"
#include "syscalls/channel.cxx"
#include "syscalls/event.cxx"
#include "syscalls/fifo.cxx"
#include "syscalls/handle.cxx"
#include "syscalls/object.cxx"
#include "syscalls/object_wait.cxx"
#include "syscalls/other.cxx"
#include "syscalls/tasks.cxx"
#include "syscalls/tests.cxx"
#include "syscalls/time.cxx"
#include "syscalls/vmo.cxx"

#include "utils/clock.cxx"
#include "utils/elf.cxx"
#include "utils/init_test.cxx"
#include "utils/page_alloc.cxx"

#include "zxcpp/pagearray.h"

/* Extra test function */
void do_cpp_test(void)
{
    using namespace ServerCxx;

    /* Test mbuf */
    MBuf buf;

    size_t len = 30;
    char test[len] = {0};
    char str[] = "HELLO HELLO HELLO";

    for (size_t i = 0; i < 10000; ++i) {
        assert(buf.write((uint8_t*)&str[0], strlen(str) + 1, true) == ZX_OK);
    }

    dprintf(SPEW, "mbuf size: %lu\n", buf.get_size());

    for (size_t i = 0; i < 10000; ++i) {
        assert(buf.read((uint8_t*)&test[0], strlen(str) + 1) == ZX_OK);
        assert(strcmp(str, &test[0]) == 0);
        memset(&test[0], 0, strlen(str) + 1);
    }

    dprintf(SPEW, "mbuf size: %lu\n", buf.get_size());

    /* Object type test */
    ZxChannel ch;
    assert(is_object_type<ZxChannel>(&ch));

    /* Page array test */
    PageArray<vka_object_t> test_pa;
    size_t pa_size = 1 * 1024 * 1024; // 1MB of pages = 4GB
    test_pa.init(pa_size);
    test_pa.alloc(0);
    test_pa.alloc(1);
    test_pa.alloc(600000);
    test_pa[0].cptr = 1;
    dprintf(INFO, "%p %lu\n", &test_pa[0], test_pa[0].cptr);
    dprintf(INFO, "%p %lu\n", &test_pa[1], test_pa[1].cptr);
    dprintf(INFO, "%p %lu\n", &test_pa[600000], test_pa[600000].cptr);

    assert(!test_pa.has(800000));

    auto clean_func = [] (vka_object_t &frame) {
        frame.cptr = 0;
    };
    test_pa.clear(clean_func);
}
