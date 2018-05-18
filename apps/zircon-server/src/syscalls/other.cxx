#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "object/process.h"
#include "sys_helpers.h"

namespace SysOther {
constexpr uint32_t kMaxDebugWriteSize = 256u;
}

uint64_t sys_undefined(seL4_MessageInfo_t tag, uint64_t badge)
{
    /* Get the syscall number */
    seL4_Word syscall = seL4_MessageInfo_get_label(tag);

    dprintf(INFO, "Received unimplemented syscall: %lu\n", syscall);
    return ZX_ERR_NOT_SUPPORTED;
}

uint64_t sys_debug_write(seL4_MessageInfo_t tag, uint64_t badge)
{
    using namespace SysOther;

    SYS_CHECK_NUM_ARGS(tag, 2);
    uintptr_t user_ptr = seL4_GetMR(0);
    uint32_t len = seL4_GetMR(1);

    if (len > kMaxDebugWriteSize) {
        len = kMaxDebugWriteSize;
    }

    zx_status_t err;
    void *buf;
    ZxProcess *proc = get_proc_from_badge(badge);

    err = proc->uvaddr_to_kvaddr(user_ptr, len, buf);
    SYS_RET_IF_ERR(err);

    dprintf(INFO, "(%s): ", proc->get_name());

    for (uint32_t i = 0; i < len; ++i) {
        putchar(((char *)buf)[i]);
    }
    putchar('\n');

    return len;
}

/* sel4zircon syscalls */
uint64_t sys_debug_putchar(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 1);
    char c = seL4_GetMR(0);
    putchar(c);
    return 0;
}

uint64_t sys_get_elf_vmo(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 5);
    zx_handle_t hrsrc = seL4_GetMR(0);
    zx_handle_t vmo_handle = seL4_GetMR(1);
    uintptr_t name_buf = seL4_GetMR(2);
    uint32_t name_len = seL4_GetMR(3);
    uintptr_t user_size = seL4_GetMR(4);

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    err = validate_resource(proc, hrsrc, ZX_RSRC_KIND_ROOT);
    SYS_RET_IF_ERR(err);

    char *filename;
    uint64_t *size;
    err = proc->uvaddr_to_kvaddr(name_buf, name_len, (void *&)filename);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_size, size);
    SYS_RET_IF_ERR(err);

    ZxVmo *vmo;
    err = proc->get_object_with_rights(vmo_handle, ZX_RIGHT_WRITE, vmo);
    SYS_RET_IF_ERR(err);

    if (!strcmp(filename, "zircon-server")) {
        return ZX_ERR_ACCESS_DENIED;
    }

    uint64_t elf_size;
    char *elf_file = get_elf_file(filename, &elf_size);
    if (elf_file == NULL) {
        return ZX_ERR_NOT_FOUND;
    }

    *size = elf_size;

    if (vmo->get_size() < elf_size) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    vmo->commit_range(0, elf_size);
    vmo->write(0, elf_size, elf_file);
    return ZX_OK;
}
