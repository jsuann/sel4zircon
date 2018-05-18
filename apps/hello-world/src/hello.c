#include <zircon/syscalls.h>
#include <zircon/types.h>

void entry(zx_handle_t channel, uintptr_t fnptr)
{
    char *hello_msg = "Hello world!";
    zx_debug_write((void *)hello_msg, 12);
    while (1);
}
