#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "sys_helpers.h"
#include "debug.h"
}

extern "C" {
void sys_process_create(seL4_MessageInfo_t tag, uint64_t badge);
void sys_process_exit(seL4_MessageInfo_t tag, uint64_t badge);
void sys_process_start(seL4_MessageInfo_t tag, uint64_t badge);
void sys_process_read_memory(seL4_MessageInfo_t tag, uint64_t badge);
void sys_process_write_memory(seL4_MessageInfo_t tag, uint64_t badge);
}

void sys_process_create(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_exit(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_start(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_read_memory(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}

void sys_process_write_memory(seL4_MessageInfo_t tag, uint64_t badge)
{
    sys_reply(ZX_ERR_NOT_SUPPORTED);
}