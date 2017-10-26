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
#pragma once

#include <vka/vka.h>
#include <vspace/vspace.h>
#include <sel4utils/thread.h>
#include <sel4utils/process.h>
#include <sel4utils/mapping.h>
#include <sel4test/test.h>

#include <sel4platsupport/timer.h>
#include <platsupport/timer.h>

#include "test.h"
#include <sel4testsupport/testreporter.h>
#include <sync/mutex.h>

#define OUR_PRIO (env->priority)
/* args provided by the user */
#define HELPER_THREAD_MAX_ARGS 4
/* metadata helpers adds */
#define HELPER_THREAD_META     4
/* total args (user + meta) */
#define HELPER_THREAD_TOTAL_ARGS (HELPER_THREAD_MAX_ARGS + HELPER_THREAD_META)

#include <sel4test/test.h>

typedef int (*helper_fn_t)(seL4_Word, seL4_Word, seL4_Word, seL4_Word);

typedef struct helper_thread {
    sel4utils_elf_region_t regions[MAX_REGIONS];
    int num_regions;
    sel4utils_process_t process;

    sel4utils_thread_t thread;
    vka_object_t local_endpoint;
    seL4_CPtr fault_endpoint;

    void *arg0;
    void *arg1;
    char *args[HELPER_THREAD_TOTAL_ARGS];
    char args_strings[HELPER_THREAD_TOTAL_ARGS][WORD_STRING_SIZE];

    bool is_process;
} helper_thread_t;

/* Helper thread/process functions */

/* create a helper in the current vspace and current cspace */
void create_helper_thread(env_t env, helper_thread_t *thread);

/* create a helper with a clone of the current vspace loadable elf segments,
 * and a new cspace */
void create_helper_process(env_t env, helper_thread_t *thread);
int create_passive_thread(env_t env, helper_thread_t *passive, helper_fn_t fn, seL4_CPtr ep,
                          seL4_Word arg1, seL4_Word arg2, seL4_Word arg3);

/* set a helper threads priority */
void set_helper_priority(env_t env, helper_thread_t *thread, seL4_Word prio);

/* set a helper threads max control priority */
void set_helper_mcp(env_t env, helper_thread_t *thread, seL4_Word mcp);

/* set a helper threads core affinity. This will have no effect on passive threads. */
void set_helper_affinity(env_t env, helper_thread_t *thread, seL4_Word affinity);

/* if CONFIG_KERNEL_RT is set, set the helpers scheduling parameters */
int set_helper_sched_params(UNUSED env_t env, UNUSED helper_thread_t *thread, UNUSED uint64_t budget,
        UNUSED uint64_t period, seL4_Word badge);

/* set a helper threads timeout fault handler */
void set_helper_tfep(env_t env, helper_thread_t *thread, seL4_CPtr tfep);

/* Start a helper. Note: arguments to helper processes will be copied into
 * the address space of that process. Do not pass pointers to data only in
 * the local vspace, this will fail. */
void start_helper(env_t env, helper_thread_t *thread, helper_fn_t entry_point,
                  seL4_Word arg0, seL4_Word arg1, seL4_Word arg2, seL4_Word arg3);

/* save a threads seL4_UserContext, increment instruction pointer, and resume */
int restart_after_syscall(env_t env, helper_thread_t *thread);

/* wait for a helper thread to finish */
int wait_for_helper(helper_thread_t *thread);

/* free all resources associated with a helper and tear it down */
void cleanup_helper(env_t env, helper_thread_t *thread);

/* retrieve the TCB of a helper thread */
seL4_CPtr get_helper_tcb(helper_thread_t *thread);
/* retrieve the reply object cap of a helper thread (seL4_CapNull if not CONFIG_RT_KERNEL) */
seL4_CPtr get_helper_reply(helper_thread_t *thread);
/* retrieve the sched context cap of a helper thread (seL4_CapNull if not CONFIG_RT_KERNEL) */
seL4_CPtr get_helper_sched_context(helper_thread_t *thread);

/* retrieve the IPC buffer address of a helper thread */
uintptr_t get_helper_ipc_buffer_addr(helper_thread_t *thread);

uintptr_t get_helper_initial_stack_pointer(helper_thread_t *thread);

/*
 * Check whether a given region of memory is zeroed out.
 */
int check_zeroes(seL4_Word addr, seL4_Word size_bytes);

/* Determine if two TCBs in the init thread's CSpace are not equal. Note that we
 * assume the thread is not currently executing.
 *
 * Serves as != comparator for caps. Returns 1 for not equal, 0 for equal and -1 for syscall error.
 */
int are_tcbs_distinct(seL4_CPtr tcb1, seL4_CPtr tcb2);

/* cnode_ops wrappers */
int cnode_copy(env_t env, seL4_CPtr src, seL4_CPtr dest, seL4_CapRights_t rights);
int cnode_delete(env_t env, seL4_CPtr slot);
int cnode_mint(env_t env, seL4_CPtr src, seL4_CPtr dest, seL4_CapRights_t rights, seL4_CapData_t badge);
int cnode_move(env_t env, seL4_CPtr src, seL4_CPtr dest);
int cnode_mutate(env_t env, seL4_CPtr src, seL4_CPtr dest);
int cnode_cancelBadgedSends(env_t env, seL4_CPtr cap);
int cnode_revoke(env_t env, seL4_CPtr cap);
int cnode_rotate(env_t env, seL4_CPtr src, seL4_CPtr pivot, seL4_CPtr dest);

/* Determine whether a given slot in the init thread's CSpace is empty by
 * examining the error when moving a slot onto itself.
 *
 * Serves as == 0 comparator for caps.
 */
int is_slot_empty(env_t env, seL4_Word slot);

/* Get a free slot */
seL4_Word get_free_slot(env_t env);

/* timer */
void wait_for_timer_interrupt(env_t env);
/* sleep for an exact time: check the time when the timeout comes in and set the timeout for further if needed.
 * Due to race conditions on waiting for interrupts sleep may *not* be used if you have started
 * created_timer_interrupt_thread. As it uses a single underlying timer and callback this cannot be used
 * by more than one thread */
void sleep(env_t env, uint64_t ns);
/* busy wait for a period of time. This assumes that you have some thread (such as create_timer_interrupt_thread)
 * handling the timer interrupts. This can be used instead of sleep in circumstances where you want multiple
 * threads performing waits */
void sleep_busy(env_t env, uint64_t ns);
/* helper for retrieving the timestamp from the timer. you must either be handling timer interrupts yourself
 * or have started create_timer_interrupt_thread */\
uint64_t timestamp(env_t env);

/* helper for creating a thread to handle timer interrupts */
int create_timer_interrupt_thread(env_t env, helper_thread_t *thread);
