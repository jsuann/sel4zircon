
#include <autoconf.h>
#include <zircon/syscalls.h>

#include "sys_helper.h"

zx_status_t zx_process_create(zx_handle_t job,
                              const char* name, uint32_t name_len,
                              uint32_t options,
                              zx_handle_t* proc_handle, zx_handle_t* vmar_handle)
{
    ZX_SYSCALL_SEND(job, ZX_SYS_PROCESS_CREATE, 4, job, (uintptr_t)name, name_len, options);
    if (seL4_GetMR(0) == ZX_OK) {
        *proc_handle = seL4_GetMR(1);
        *vmar_handle = seL4_GetMR(2);
    }
    return seL4_GetMR(0);
}

zx_status_t zx_process_start(zx_handle_t process, zx_handle_t thread,
                            uintptr_t entry, uintptr_t stack,
                            zx_handle_t arg1, uintptr_t arg2)
{
    ZX_SYSCALL_SEND_WITH_CAPS(process, ZX_SYS_PROCESS_START, 2, 3, thread, arg1, entry, stack, arg2);
    return seL4_GetMR(0);
}

void zx_process_exit(int ret_code)
{
    ZX_SYSCALL_SEND(DEFAULT_HANDLE_CPTR, ZX_SYS_PROCESS_EXIT, 1, ret_code);
}
