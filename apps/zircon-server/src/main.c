/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <vka/object.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>
#include <allocman/cspaceops.h>

#include <vspace/vspace.h>

#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>
#include <sel4utils/process.h>

#include <sel4platsupport/timer.h>
#include <sel4platsupport/bootinfo.h>
#include <platsupport/plat/timer.h>

#include "debug.h"
#include "syscalls.h"
#include "sys_helpers.h"

/* constants */
#define EP_BADGE 0x61 // arbitrary (but unique) number for a badge
#define MSG_DATA 0x6161 // arbitrary data to send

#define APP_PRIORITY seL4_MaxPrio
#define APP_IMAGE_NAME "zircon-test"

/* Use a two level cspace to allow for higher memory usage */
/* A patch for libsel4simple-default is required for this to work */
#define ZX_USE_TWO_LEVEL_CSPACE     1

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE      (BIT(seL4_PageBits) * 40)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE     (BIT(seL4_PageBits) * 32000)
#define ALLOCATOR_VIRTUAL_POOL_START    0x10000000ul

/* static memory for virtual memory bootstrapping */
UNUSED static sel4utils_alloc_data_t data;

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 4096
UNUSED static int thread_2_stack[THREAD_2_STACK_SIZE];

/* convenience function */
extern void name_thread(seL4_CPtr tcb, char *name);

/* zircon server calls */
extern void do_cpp_test(void);
extern void init_zircon_server(vka_t *vka, vspace_t *vspace, seL4_CPtr new_ep);
extern uint64_t init_zircon_test(void);
extern void send_zircon_test_data(seL4_CPtr ep_cap);
extern void syscall_loop(void);

int main(void) {
    int error;
    seL4_BootInfo *info;
    simple_t simple;
    vka_t vka;
    allocman_t *allocman;
    vspace_t vspace;
    seL4_timer_t timer;

    /* get boot info */
    info = platsupport_get_bootinfo();
    ZF_LOGF_IF(info == NULL, "Failed to get bootinfo.");

    /* give us a name: useful for debugging if the thread faults */
    name_thread(seL4_CapInitThreadTCB, "zircon-server");

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    //simple_print(&simple);

    dprintf(ALWAYS, "cap count: %d\n", simple_get_cap_count(&simple));
    dprintf(ALWAYS, "empty: %lu, %lu\n", info->empty.start, info->empty.end);
    dprintf(ALWAYS, "shared frames: %lu, %lu\n", info->sharedFrames.start, info->sharedFrames.end);
    dprintf(ALWAYS, "io space caps: %lu, %lu\n", info->ioSpaceCaps.start, info->ioSpaceCaps.end);
    dprintf(ALWAYS, "user img frames: %lu, %lu\n", info->userImageFrames.start, info->userImageFrames.end);
    dprintf(ALWAYS, "user img paging: %lu, %lu\n", info->userImagePaging.start, info->userImagePaging.end);
    dprintf(ALWAYS, "Extra boot info frames: %lu, %lu\n", info->extraBIPages.start, info->extraBIPages.end);
    dprintf(ALWAYS, "untyped: %lu, %lu\n", info->untyped.start, info->untyped.end);

    size_t cap_count = simple_get_cap_count(&simple);
    for (size_t i = 0; i < cap_count; ++i) {
        seL4_CPtr pos = simple_get_nth_cap(&simple,i);
        if (pos < i)
            dprintf(ALWAYS, "BAD: %luth cap, cap pos: %lu\n", i, pos);
    }

#if ZX_USE_TWO_LEVEL_CSPACE
    allocman = bootstrap_new_2level_simple(&simple, 14, 14, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    assert(allocman);

    allocman_make_vka(&vka, allocman);

    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace, &data, seL4_CapInitThreadPD, &vka, info);
    assert(!error);
#else
    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    assert(allocman);

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);

    /* create a vspace object to manage our vspace */
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace, &data, simple_get_pd(&simple), &vka, info);
    assert(!error);
#endif

    /* fill the allocator with virtual memory */
    void *vaddr = (void *)ALLOCATOR_VIRTUAL_POOL_START;
    UNUSED reservation_t virtual_reservation;
    virtual_reservation = vspace_reserve_range_at(&vspace, vaddr, ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1);
    assert(virtual_reservation.res);
    bootstrap_configure_virtual_pool(allocman, vaddr,
                                     ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&simple));
    error = allocman_fill_reserves(allocman);
    assert(!error);


    /* ---- HERE ONWARDS ---- */

    /* Use zircon structures! */



    /* use sel4utils to make a new process */
    sel4utils_process_t new_process;
    sel4utils_process_config_t config = process_config_default_simple(&simple, APP_IMAGE_NAME, APP_PRIORITY);
    error = sel4utils_configure_process_custom(&new_process, &vka, &vspace, config);
    assert(error == 0);

    /* give the new process's thread a name */
    name_thread(new_process.thread.tcb.cptr, "zircon-test");

    /* create an endpoint */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    assert(error == 0);

    init_zircon_server(&vka, &vspace, ep_object.cptr);
    uint64_t badge_val = init_zircon_test();

    /*
     * make a badged endpoint in the new process's cspace.  This copy
     * will be used to send an IPC to the original cap
     */

    /* make a cspacepath for the new endpoint cap */
    cspacepath_t ep_cap_path;
    seL4_CPtr new_ep_cap;
    vka_cspace_make_path(&vka, ep_object.cptr, &ep_cap_path);

    /* copy the endpont cap and add a badge to the new cap */
    new_ep_cap = sel4utils_mint_cap_to_process(&new_process, ep_cap_path, seL4_AllRights, seL4_CapData_Badge_new(badge_val));
    assert(new_ep_cap != 0);

    /* spawn the process */
    error = sel4utils_spawn_process_v(&new_process, &vka, &vspace, 0, NULL, 1);
    assert(error == 0);

    vka_object_t ntfn_object = {0};
    error = vka_alloc_notification(&vka, &ntfn_object);
    assert(error == 0);

    error = sel4platsupport_init_default_timer(&vka, &vspace, &simple, ntfn_object.cptr, &timer);
    assert(error == 0);

    dprintf(ALWAYS, "=== Zircon Server ===\n");

    //seL4_Word sender_badge = 0;
    //seL4_MessageInfo_t tag;
    //seL4_Word msg;

/*
    error = ltimer_set_timeout(&timer.ltimer, NS_IN_MS, TIMEOUT_PERIODIC);
    assert(error == 0);

    int count = 0;
    while (1) {

        seL4_Word badge;
        seL4_Wait(ntfn_object.cptr, &badge);
        sel4platsupport_handle_timer_irq(&timer, badge);
        printf("badge: %lu\n", badge);

        count++;
        if (count == 10 * msg) {
            break;
        }
    }

    uint64_t time;
    ltimer_get_time(&timer.ltimer, &time);

    msg = (uint32_t) time;
    seL4_SetMR(0, msg);
    seL4_Reply(tag);
*/

    sel4platsupport_destroy_timer(&timer, &vka);

    do_cpp_test();
    send_zircon_test_data(ep_cap_path.capPtr);

    syscall_loop();

    return 0;
}
