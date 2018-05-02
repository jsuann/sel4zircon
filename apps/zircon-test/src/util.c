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

/*
 * helper functions
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <sel4/sel4.h>
#include <sel4zircon/debug.h>

/* avoid main falling off the end of the world */
void abort(void) {
    while (1);
}

/* enable printf to use kernel debug printing */
void __arch_putchar(int c) {
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugPutChar(c);
#else
    //zx_debug_write((void *)&c, 1);
    zx_debug_putchar(c);
#endif
}

/* set a thread's name for debugging purposes */
void name_thread(seL4_CPtr tcb, char *name) {
#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(tcb, name);
#endif
}
