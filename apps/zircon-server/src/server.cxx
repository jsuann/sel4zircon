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

#include "utils/elf.h"
#include "utils/rng.h"
#include "utils/page_alloc.h"

/* Wrap globals in a namespace to prevent access outside this file */
namespace ServerCxx {

/* seL4 stuff used by server */
vka_t *server_vka;
vspace_t *server_vspace;
seL4_CPtr server_ep;
seL4_timer_t *server_timer;

seL4_Word timer_badge;

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

void init_zircon_server(vka_t *vka, vspace_t *vspace,
        seL4_timer_t *timer, seL4_CPtr new_ep, seL4_CPtr ntfn)
{
    using namespace ServerCxx;

    dprintf(ALWAYS, "=== Zircon Server ===\n");

    /* store server globals */
    server_vka = vka;
    server_vspace = vspace;
    server_ep = new_ep;
    server_timer = timer;

    /* init allocators and other things */
    init_handle_table(server_vspace);
    init_proc_table(server_vspace);
    init_thread_table(server_vspace);
    init_vmo_kmap();
    init_prng();
    init_root_job();
    init_asid_pool(server_vka);
    init_page_alloc(server_vka);

    /* save the timer badge. a bit hacky but we know x86 has one irq */
    /* XXX newer versions of QEMU seem to constantly fire interrupts, in addition
       to when we program the hpet to fire. Don't use hpet with TIMEOUT_PERIODIC */
    assert(ltimer_set_timeout(&timer->ltimer, 1 * NS_IN_MS, TIMEOUT_RELATIVE) == 0);
    seL4_Wait(ntfn, &timer_badge);
    sel4platsupport_handle_timer_irq(timer, timer_badge);
    dprintf(INFO, "Timer badge: %lu\n", timer_badge);

    /* Bind notification to the tcb */
    assert(seL4_TCB_BindNotification(seL4_CapInitThreadTCB, ntfn) == 0);
}

void syscall_loop(void)
{
    using namespace ServerCxx;

    seL4_Word badge = 0;
    seL4_MessageInfo_t tag;

    /* XXX test timer tick */
    uint64_t timeacc = 0;
    uint64_t prevtime = 0;
    uint64_t count = 0;

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
        } else if (badge == timer_badge) {
            /* Received timer interrupt */
            sel4platsupport_handle_timer_irq(server_timer, timer_badge);
            //dprintf(INFO, "Got timer IRQ! Badge %lu\n", timer_badge);

            /* XXX test timer tick */
            uint64_t time;
            ltimer_get_time(&server_timer->ltimer, &time);
            timeacc += (time - prevtime);
            prevtime = time;
            ++count;
            if (count == 5000) {
                dprintf(INFO, "Tick is %lu\n", (timeacc/count));
                assert(!"dead");
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
#include "object/socket.cxx"

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
