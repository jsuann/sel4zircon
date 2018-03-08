#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include "zxcpp/new.h"

extern "C" {
#include <zircon/types.h>
#include "debug.h"
}

#include "object/handle.h"
#include "object/process.h"

/*
 * We need to include other cxx files in subdirs, rather than compile
 * them separately. This is due to enums defined in sel4/sel4.h causing
 * linker issues.
 */
#include "syscalls/channel.cxx"
#include "syscalls/handle.cxx"
#include "syscalls/other.cxx"
#include "syscalls/tasks.cxx"
#include "syscalls/tests.cxx"

extern "C" void do_cpp_test(void);

void do_cpp_test(void)
{
    /* For now, just malloc handles */
    ZxProcess *p1 = allocate<ZxProcess>(9999);
    Handle *h1 = allocate<Handle>(p1, p1, ZX_RIGHT_READ, 262626);
    dprintf(SPEW, "%p %p\n", h1, p1);

    ZxObject *o1 = p1;
    dprintf(SPEW, "Type of p1: %u, should be: %u\n", p1->get_object_type(), ZX_OBJ_TYPE_PROCESS);

    dprintf(SPEW, "Type of o1: %u, should be: %u\n", o1->get_object_type(), ZX_OBJ_TYPE_PROCESS);

    ZxObject *o2 = allocate<ZxObject>(13123);
    dprintf(SPEW, "Type of o2: %u, should be: %u\n", o2->get_object_type(), ZX_OBJ_TYPE_NONE);

    delete h1;
    delete p1;
}
