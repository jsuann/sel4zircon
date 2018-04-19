#pragma once

#include <autoconf.h>

/* Layout for a ZxThread Cspace */
enum ZxThreadCspaceSlots {
    /* No cap in NULL */
    ZX_THREAD_NULL_SLOT = 0,
    /* Fault endpoint slot */
    ZX_THREAD_FAULT_SLOT = 1,
    /* Regular syscall endpoint slot */
    ZX_THREAD_SYSCALL_SLOT = 2,
    /* First free slot in the cspace */
    ZX_THREAD_FIRST_FREE = 3
};
