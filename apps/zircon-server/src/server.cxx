#include <autoconf.h>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "addrspace.h"
#include "zxcpp/new.h"

extern "C" {
#include <vka/object.h>
#include <zircon/types.h>
#include <vspace/vspace.h>
#include "debug.h"
}

#include "object/handle.h"
#include "object/process.h"
#include "object/vmar.h"

extern "C" void do_cpp_test(void);
extern "C" void init_zircon_server(vka_t *vka, vspace_t *vspace);
extern "C" uint64_t init_zircon_test(void);
extern "C" void send_zircon_test_data(seL4_CPtr ep_cap);

/* seL4 interfaces used by server */
vka_t *server_vka;
vspace_t *server_vspace;

/* Base zx objects for zircon test */
ZxProcess *test_proc;
ZxVmar *test_vmar;

void init_zircon_server(vka_t *vka, vspace_t *vspace)
{
    server_vka = vka;
    server_vspace = vspace;

    init_handle_table(server_vspace);
    init_proc_table(server_vspace);
    init_vmo_kmap();
}

uint64_t init_zircon_test(void)
{
    /* Create a root vmar */
    test_vmar = allocate_object<ZxVmar>();
    assert(test_vmar != NULL);

    /* Create a thread */

    /* Create a process */
    test_proc = allocate_object<ZxProcess>(test_vmar);
    test_proc->set_name("zircon-test");

    /* Create VMOs */

    /* Return badge value for process */
    return test_proc->get_badge_val();
}

void send_zircon_test_data(seL4_CPtr ep_cap)
{
    /* Get handles to test objects */
    Handle *vmar_handle = test_vmar->create_handle(ZX_RIGHTS_IO);
    Handle *proc_handle = test_proc->create_handle(ZX_RIGHTS_BASIC);

    /* Add handles to process */
    test_proc->add_handle(vmar_handle);
    test_proc->add_handle(proc_handle);
    test_proc->print_handles();

    /* Get user handle values */
    zx_handle_t vmar_uval = test_proc->get_handle_user_val(vmar_handle);
    zx_handle_t proc_uval = test_proc->get_handle_user_val(proc_handle);

    /* Send handles to zircon test */
    dprintf(SPEW, "Sending test data to zircon test!\n");
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, vmar_uval);
    seL4_SetMR(1, proc_uval);
    seL4_Send(ep_cap, tag);
}

void do_cpp_test(void)
{
    dprintf(SPEW, "Root vmar base: %lx, size: %lx, end: %lx\n", root_base, root_size, (root_base+root_size));

    /* Test allocation */
    void *ptr;
    ptr = malloc(10000);
    dprintf(SPEW, "malloc ptr at %p\n", ptr);
    free(ptr);

    vspace_new_pages_config_t config;
    default_vspace_new_pages_config(1, seL4_PageBits, &config);
    vspace_new_pages_config_set_vaddr((void *)ZX_VMO_SERVER_MAP_START, &config);

    ptr = vspace_new_pages_with_config(server_vspace, &config, seL4_AllRights);
    dprintf(SPEW, "vspace page ptr at %p\n", ptr);
    vspace_unmap_pages(server_vspace, (void *)ZX_VMO_SERVER_MAP_START, 1, seL4_PageBits, VSPACE_FREE);

    ptr = vspace_new_pages_with_config(server_vspace, &config, seL4_AllRights);
    dprintf(SPEW, "vspace page ptr at %p\n", ptr);
    vspace_unmap_pages(server_vspace, (void *)ZX_VMO_SERVER_MAP_START, 1, seL4_PageBits, VSPACE_FREE);

    dprintf(SPEW, "address of ptr at %p\n", &ptr);

    uintptr_t vmo_kmap = alloc_vmo_kmap();
    free_vmo_kmap(vmo_kmap);

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

/*
 * We need to include other cxx files in subdirs, rather than compile
 * them separately. This is due to enums defined in sel4/sel4.h causing
 * linker issues.
 */
#include "object/handle.cxx"
#include "object/object.cxx"
#include "object/process.cxx"

#include "syscalls/channel.cxx"
#include "syscalls/handle.cxx"
#include "syscalls/other.cxx"
#include "syscalls/tasks.cxx"
#include "syscalls/tests.cxx"
