#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <zircon/types.h>

#include "object.h"

typedef struct zir_vmar {
    // obj info
    zir_object_t obj;
} zir_vmar_t;
