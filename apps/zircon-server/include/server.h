#pragma once

extern "C" {
#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <vka/object.h>
}

/* server ep badge bits (28 bits) */
constexpr seL4_Word ZxFaultBadge = 1 << 24;
constexpr seL4_Word ZxIrqBadge = 1 << 23;
constexpr seL4_Word ZxSyscallBadge = 1 << 22;

/* Mask to remove above bits */
constexpr seL4_Word ZxBadgeMask = (1 << 20) - 1;

seL4_CPtr get_server_ep();
vspace_t *get_server_vspace();
vka_t *get_server_vka();
