#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4utils/process.h>
#include <zircon/types.h>

#include "object.h"

typedef struct zir_process {
    // obj info
    zir_object_t obj;
    // TODO process stuff
    //zir_handle_t *handle_list;
    // vmar aspace

    // sel4 stuff
    // TODO probs cspace stuff
    // thread list

    // XXX for now, just use existing sel4utils process
    //sel4utils_process_t sel4_proc;
} zir_process_t;


// XXX creating a process: make sure to load in
// a default handle in every process
// this is used for syscalls without a 


static inline void init_zir_process(zir_process_t *proc)
{
    init_zir_object(&(proc->obj), ZIR_PROCESS);
    printf("%p  %p \n", proc, &(proc->obj));
}
