#include <zircon/syscalls.h>
#include <zircon/types.h>

void entry(zx_handle_t channel, uintptr_t fnptr)
{
    char *hello_msg = "Hello world!";
    zx_debug_write((void *)hello_msg, 12);
    zx_nanosleep(zx_deadline_after(ZX_MSEC(500)));
    zx_process_exit(0);
}
