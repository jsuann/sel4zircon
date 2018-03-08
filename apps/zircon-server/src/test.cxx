#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <zircon/types.h>

#include "zxcpp/new.h"
#include "object/handle.h"
#include "object/process.h"

extern "C" void do_cpp_test(void);

void do_cpp_test(void)
{
    /* For now, just malloc handles */
    ZxProcess *p1 = allocate<ZxProcess>(9999);
    Handle *h1 = allocate<Handle>(p1, p1, ZX_RIGHT_READ, 262626);
    printf("%p %p\n", h1, p1);
    
    printf("Type of p1: %u, should be: %u\n", p1->get_object_type(), ZX_OBJ_TYPE_PROCESS);

    delete h1;
    delete p1;
}
