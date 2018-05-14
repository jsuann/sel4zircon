#include <autoconf.h>

extern "C" {
#include "debug.h"
}

#include "sys_helpers.h"
#include "utils/system.h"

uint64_t sys_system_get_num_cpus(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 0);
    return system_get_num_cores();
}
