#pragma once

#include <autoconf.h>

extern "C" {
#include <zircon/types.h>
#include <sel4/sel4.h>
#include "debug.h"
}

void handle_fault(seL4_MessageInfo_t tag, uint64_t badge);
