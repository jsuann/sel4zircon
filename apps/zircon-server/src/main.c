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

/* TODO make defined flags config options */

/* Use a two level cspace to allow for higher memory usage */
/* A patch for libsel4simple-default is required for this to work */
#define ZX_USE_TWO_LEVEL_CSPACE         1

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE      (BIT(seL4_PageBits) * 40)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* static memory for virtual memory bootstrapping */
UNUSED static sel4utils_alloc_data_t data;

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE     (BIT(seL4_PageBits) * 6000)
#define ALLOCATOR_VIRTUAL_POOL_START    0x10000000ul

#define ZX_SERVER_STACK_START           0x30000000ul
#define ZX_SERVER_STACK_NUM_PAGES       14

/* Use a bigger static heap for the server vs. other processes.
   Requires libsel4muslcsys patch. */
#define ZX_USE_CUSTOM_HEAP              1

#if ZX_USE_CUSTOM_HEAP
/* Function in sys_morecore.c to change heap */
extern void change_morecore_area(void *base, size_t size);

/* Dimensions of heap */
#define ZX_SERVER_HEAP_SIZE             0x2000000ul /* 32 MB */

/* Heap decl */
char server_heap[ZX_SERVER_HEAP_SIZE];
#endif /* ZX_USE_CUSTOM_HEAP */

/* zircon server calls */
extern void do_cpp_test(void);
extern void init_zircon_server(vka_t *vka, vspace_t *vspace, seL4_CPtr new_ep);
extern uint64_t init_zircon_test(void);
extern void send_zircon_test_data(seL4_CPtr ep_cap);
extern void syscall_loop(void);

void *main_continued(void *arg);

int main(void) {
    int error;
    seL4_BootInfo *info;
    simple_t simple;
    vka_t vka;
    allocman_t *allocman;
    vspace_t vspace;
    seL4_timer_t timer;
    vspace_new_pages_config_t config;

#if ZX_USE_CUSTOM_HEAP
    dprintf(SPEW, "Changing heap: base %p, size %lu\n", &server_heap, ZX_SERVER_HEAP_SIZE);
    change_morecore_area(&server_heap, ZX_SERVER_HEAP_SIZE);
#endif /* ZX_USE_CUSTOM_HEAP */

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
    //simple_print(&simple);

    /* Additional bootinfo cap prints */
    dprintf(SPEW, "cap count: %d\n", simple_get_cap_count(&simple));
    dprintf(SPEW, "empty: %lu, %lu\n", info->empty.start, info->empty.end);
    dprintf(SPEW, "shared frames: %lu, %lu\n", info->sharedFrames.start, info->sharedFrames.end);
    dprintf(SPEW, "io space caps: %lu, %lu\n", info->ioSpaceCaps.start, info->ioSpaceCaps.end);
    dprintf(SPEW, "user img frames: %lu, %lu\n", info->userImageFrames.start, info->userImageFrames.end);
    dprintf(SPEW, "user img paging: %lu, %lu\n", info->userImagePaging.start, info->userImagePaging.end);
    dprintf(SPEW, "Extra boot info frames: %lu, %lu\n", info->extraBIPages.start, info->extraBIPages.end);
    dprintf(SPEW, "untyped: %lu, %lu\n", info->untyped.start, info->untyped.end);

    size_t cap_count = simple_get_cap_count(&simple);
    for (size_t i = 0; i < cap_count; ++i) {
        seL4_CPtr pos = simple_get_nth_cap(&simple,i);
        if (pos < i)
            dprintf(SPEW, "BAD: %luth cap, cap pos: %lu\n", i, pos);
    }

#if ZX_USE_TWO_LEVEL_CSPACE
    /* create new 2 level cspace & allocator */
    allocman = bootstrap_new_2level_simple(&simple, 14, 14, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    assert(allocman);

    /* create a vka */
    allocman_make_vka(&vka, allocman);

    /* create vspace manager */
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&vspace, &data, seL4_CapInitThreadPD, &vka, info);
    assert(!error);
#else
    /* create an allocator */
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    assert(allocman);

    /* create a vka */
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
    bootstrap_configure_virtual_pool(allocman, vaddr, ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&simple));
    error = allocman_fill_reserves(allocman);
    assert(!error);

    /* Create an endpoint for zircon server to wait on */
    vka_object_t ep_object = {0};
    error = vka_alloc_endpoint(&vka, &ep_object);
    assert(error == 0);

    /* TODO use this for zircon timer objects, waiting, etc. */
    vka_object_t ntfn_object = {0};
    error = vka_alloc_notification(&vka, &ntfn_object);
    assert(error == 0);
    error = sel4platsupport_init_default_timer(&vka, &vspace, &simple, ntfn_object.cptr, &timer);
    assert(error == 0);

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
        if (count == 10) {
            break;
        }
    }

    uint64_t time;
    ltimer_get_time(&timer.ltimer, &time);

    msg = (uint32_t) time;
    seL4_SetMR(0, msg);
    seL4_Reply(tag);

    sel4platsupport_destroy_timer(&timer, &vka);
    while (1);
*/

    /* Init the zircon server */
    init_zircon_server(&vka, &vspace, ep_object.cptr);

    /* Init zircon test */
    init_zircon_test();

    /* Configure new server stack */
    default_vspace_new_pages_config(ZX_SERVER_STACK_NUM_PAGES, seL4_PageBits, &config);
    vspace_new_pages_config_set_vaddr((void *)ZX_SERVER_STACK_START, &config);

    /* Allocate server stack */
    void *stack_base = vspace_new_pages_with_config(&vspace, &config, seL4_AllRights);
    assert(stack_base != NULL);

    /* Run on new stack */
    void *stack_top = (void *)(ZX_SERVER_STACK_START + (ZX_SERVER_STACK_NUM_PAGES * BIT(seL4_PageBits)));
    utils_run_on_stack(stack_top, main_continued, NULL);

    /* We shouldn't get here! */
    return 0;
}

void *main_continued(void *arg UNUSED)
{
    do_cpp_test();

    /* Enter syscall loop */
    syscall_loop();

    return NULL;
}
