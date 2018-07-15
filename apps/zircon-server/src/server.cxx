#include <autoconf.h>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "server.h"
#include "addrspace.h"
#include "zxcpp/new.h"

extern "C" {
#include <zircon/types.h>
#include <zircon/rights.h>
#include "env.h"
#include "debug.h"
}

#include "syscalls.h"
#include "sys_helpers.h"

#include "object/job.h"
#include "object/handle.h"
#include "object/vmar.h"
#include "object/mbuf.h"
#include "object/resource.h"

#include "utils/clock.h"
#include "utils/elf.h"
#include "utils/fault.h"
#include "utils/rng.h"
#include "utils/page_alloc.h"
#include "utils/system.h"

/* Wrap globals in a namespace to prevent access outside this file */
namespace ServerCxx {

/* seL4 stuff used by server */
vka_t *server_vka;
vspace_t *server_vspace;
seL4_CPtr server_ep;
seL4_Word timer_badge;

bool should_reply;

} /* namespace ServerCxx */

extern "C" void init_zircon_server(env_t *env);
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

void init_zircon_server(env_t *env)
{
    using namespace ServerCxx;

    dprintf(ALWAYS, "=== Zircon Server ===\n");

    /* store server globals */
    server_vka = env->vka;
    server_vspace = env->vspace;
    server_ep = env->server_ep;

    /* Init timer */
    init_timer(env->timer, env->timer_ntfn,
            seL4_CapInitThreadTCB, &timer_badge);

    /* init allocators and other things */
    init_handle_table(server_vspace);
    init_proc_table(server_vspace);
    init_thread_table(server_vspace);
    init_vmo_kmap();
    init_prng();
    init_root_job();
    init_root_resource();
    init_asid_pool(server_vka);
    init_page_alloc(server_vka);

    init_system_info(env);
}

void syscall_loop(void)
{
    using namespace ServerCxx;

    uint64_t badge, ret;
    seL4_MessageInfo_t tag;

    /* Wait for the first message */
    tag = seL4_Recv(server_ep, &badge);
    for (;;) {
        /* Handle the received message */
        should_reply = true;
        if (badge & ZxSyscallBadge) {
            seL4_Word syscall = seL4_MessageInfo_get_label(tag);
            if (unlikely(syscall >= NUM_SYSCALLS)) {
                /* syscall doesn't exist */
                tag = get_reply(ZX_ERR_BAD_SYSCALL);
            } else {
                ret = DO_SYSCALL(syscall, tag, badge);
                /* nearly all syscalls reply immediately */
                if (likely(should_reply)) {
                    tag = get_reply(ret);
                }
            }
        } else if (badge & ZxFaultBadge) {
            /* We expect that most faults are VM faults and can be handled */
            should_reply = handle_fault(tag, badge);
            if (likely(should_reply)) {
                /* Fault successfully handled, we can reply to thread */
                tag = seL4_MessageInfo_new(0, 0, 0, 0);
            }
        } else if (badge == timer_badge) {
            /* Handle timer interrupt */
            handle_timer(badge);
            should_reply = false;
        } else {
            /* We got some other message. For now, we ignore caller, and
               don't respond. This can be modified later to allow for
               communication with native seL4 processes. */
            should_reply = false;
        }

        /* Reply to previous caller if required, and wait for next event */
        if (should_reply) {
            tag = seL4_ReplyRecv(server_ep, tag, &badge);
        } else {
            tag = seL4_Recv(server_ep, &badge);
        }
    }
}

/*
 * We need to include other cxx files in subdirs, rather than compile
 * them separately. This is due to enums defined in sel4/sel4.h causing
 * linker issues. Namespaces are used to provide some inter-file safety.
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
#include "object/resource.cxx"
#include "object/event.cxx"
#include "object/endpoint.cxx"
#include "object/tasks.cxx"
#include "object/timer.cxx"

#include "syscalls/sys_table.cxx"
#include "syscalls/channel.cxx"
#include "syscalls/clock.cxx"
#include "syscalls/endpoint.cxx"
#include "syscalls/event.cxx"
#include "syscalls/fifo.cxx"
#include "syscalls/handle.cxx"
#include "syscalls/object.cxx"
#include "syscalls/object_wait.cxx"
#include "syscalls/other.cxx"
#include "syscalls/socket.cxx"
#include "syscalls/system.cxx"
#include "syscalls/tasks.cxx"
#include "syscalls/tests.cxx"
#include "syscalls/timer.cxx"
#include "syscalls/vmar.cxx"
#include "syscalls/vmo.cxx"

#include "utils/clock.cxx"
#include "utils/elf.cxx"
#include "utils/fault.cxx"
#include "utils/init_test.cxx"
#include "utils/page_alloc.cxx"
#include "utils/system.cxx"
