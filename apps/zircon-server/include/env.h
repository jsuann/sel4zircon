#pragma once

#include <autoconf.h>

#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <sel4platsupport/timer.h>
#include <vka/object.h>

/* Struct to transfer initial env to server */
struct env {
    vka_t *vka;
    vspace_t *vspace;
    seL4_timer_t *timer;
    seL4_CPtr server_ep;
    seL4_CPtr timer_ntfn;
    uint64_t tsc_freq;
    uint64_t num_cores;
    uint64_t phys_mem;
};

typedef struct env env_t;
