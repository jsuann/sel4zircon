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

#include <sel4platsupport/platsupport.h>
#include <sel4platsupport/timer.h>
#include <sel4platsupport/bootinfo.h>
#include <platsupport/plat/timer.h>

#include "env.h"
#include "debug.h"

/* Use a two level cspace to allow for higher memory usage */
/* A patch for libsel4simple-default is required for this to work */
#define ZX_USE_TWO_LEVEL_CSPACE         1

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE      (BIT(seL4_PageBits) * 100)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE     (BIT(seL4_PageBits) * 6000)
#define ALLOCATOR_VIRTUAL_POOL_START    0x10000000ul

#define ZX_SERVER_STACK_START           0x1f000000ul
#define ZX_SERVER_STACK_NUM_PAGES       20

/* server structs */
seL4_BootInfo *info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;
seL4_timer_t timer;

/* zircon server calls */
extern void init_zircon_server(env_t *env);
extern uint64_t init_zircon_test(void);
extern void syscall_loop(void);

void *main_continued(void *arg);

int main(void)
{
    int error;
    vspace_new_pages_config_t config;
    sel4utils_alloc_data_t data;

    /* get boot info */
    info = platsupport_get_bootinfo();
    ZF_LOGF_IF(info == NULL, "Failed to get bootinfo.");

    /* give us a name: useful for debugging if the thread faults */
#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(seL4_CapInitThreadTCB, "zircon-server");
#endif

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    /* simple_print(&simple); */

    /* Additional bootinfo cap prints */
#ifdef SEL4_DEBUG_KERNEL
    dprintf(SPEW, "cap count: %d\n", simple_get_cap_count(&simple));
    dprintf(SPEW, "empty: %lu, %lu\n", info->empty.start, info->empty.end);
    dprintf(SPEW, "shared frames: %lu, %lu\n", info->sharedFrames.start,
            info->sharedFrames.end);
    dprintf(SPEW, "io space caps: %lu, %lu\n", info->ioSpaceCaps.start,
            info->ioSpaceCaps.end);
    dprintf(SPEW, "user img frames: %lu, %lu\n", info->userImageFrames.start,
            info->userImageFrames.end);
    dprintf(SPEW, "user img paging: %lu, %lu\n", info->userImagePaging.start,
            info->userImagePaging.end);
    dprintf(SPEW, "Extra boot info frames: %lu, %lu\n", info->extraBIPages.start,
            info->extraBIPages.end);
    dprintf(SPEW, "untyped: %lu, %lu\n", info->untyped.start, info->untyped.end);
#endif

    size_t cap_count = simple_get_cap_count(&simple);

    for (size_t i = 0; i < cap_count; ++i) {
        seL4_CPtr pos = simple_get_nth_cap(&simple, i);

        if (pos < i) {
            dprintf(SPEW, "BAD: %luth cap, cap pos: %lu\n", i, pos);
        }
    }

#if ZX_USE_TWO_LEVEL_CSPACE
    /* create new 2 level cspace & allocator */
    allocman = bootstrap_new_2level_simple(&simple, 14, 14,
                    ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    assert(allocman);

    /* create a vka */
    allocman_make_vka(&vka, allocman);

    /* create vspace manager */
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace, &data,
                    seL4_CapInitThreadPD, &vka, info);
    assert(!error);
#else
    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE,
                    allocator_mem_pool);
    assert(allocman);

    /* create a vka */
    allocman_make_vka(&vka, allocman);

    /* create a vspace object to manage our vspace */
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace, &data,
                    simple_get_pd(&simple), &vka, info);
    assert(!error);
#endif

    /* fill the allocator with virtual memory */
    void *vaddr = (void *)ALLOCATOR_VIRTUAL_POOL_START;
    UNUSED reservation_t virtual_reservation;
    virtual_reservation = vspace_reserve_range_at(&vspace, vaddr,
                    ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1);
    assert(virtual_reservation.res);
    bootstrap_configure_virtual_pool(allocman, vaddr, ALLOCATOR_VIRTUAL_POOL_SIZE,
            simple_get_pd(&simple));
    error = allocman_fill_reserves(allocman);
    assert(!error);

    /* Configure serial driver */
    error = platsupport_serial_setup_simple(&vspace, &simple, &vka);
    assert(!error);

    /* Create an endpoint for zircon server to wait on */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    assert(error == 0);

    /* Create ntfn and init timer */
    vka_object_t ntfn_object = {0};
    error = vka_alloc_notification(&vka, &ntfn_object);
    assert(error == 0);
    error = sel4platsupport_init_default_timer(&vka, &vspace, &simple,
                    ntfn_object.cptr, &timer);
    assert(error == 0);

    /* Calc available phys mem size from untypeds */
    uint64_t phys_mem = 0;
    int untyped_count = simple_get_untyped_count(&simple);

    for (int i = 0; i < untyped_count; ++i) {
        size_t size_bits;
        simple_get_nth_untyped(&simple, i, &size_bits, NULL, NULL);
        phys_mem += (1 << size_bits);
    }

    /* Set up server env */
    env_t server_env = {
        .vka = &vka,
        .vspace = &vspace,
        .timer = &timer,
        .server_ep = ep_object.cptr,
        .timer_ntfn = ntfn_object.cptr,
        .tsc_freq = simple_get_arch_info(&simple),
        .num_cores = simple_get_core_count(&simple),
        .phys_mem = phys_mem,
    };

    /* Init the zircon server */
    init_zircon_server(&server_env);

    /* Init zircon test */
    init_zircon_test();

    /* Configure new server stack */
    default_vspace_new_pages_config(ZX_SERVER_STACK_NUM_PAGES, seL4_PageBits,
            &config);
    vspace_new_pages_config_set_vaddr((void *)ZX_SERVER_STACK_START, &config);

    /* Allocate server stack */
    void *stack_base = vspace_new_pages_with_config(&vspace, &config,
                    seL4_AllRights);
    assert(stack_base != NULL);

    /* Run on new stack */
    void *stack_top = (void *)(ZX_SERVER_STACK_START + (ZX_SERVER_STACK_NUM_PAGES *
                            BIT(seL4_PageBits)));
    fflush(stdout);
    utils_run_on_stack(stack_top, main_continued, NULL);

    /* We shouldn't get here! */
    return 0;
}

void *main_continued(void *arg UNUSED)
{
    /* Enter syscall loop */
    syscall_loop();

    return NULL;
}
