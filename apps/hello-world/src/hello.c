#include <zircon/syscalls.h>
#include <zircon/types.h>
#include <assert.h>

#define CHANNEL_BUF_SIZE    16384u

void entry(zx_handle_t channel, uintptr_t fnptr)
{
    //char *hello_msg = "Hello world!";
    //zx_debug_write((void *)hello_msg, 12);

    char buf[CHANNEL_BUF_SIZE];// = {0};
    uint32_t actual;

    while (1) {
        zx_object_wait_one(channel, ZX_CHANNEL_READABLE,  ZX_TIME_INFINITE, NULL);
        zx_channel_read(channel, 0, buf, NULL, CHANNEL_BUF_SIZE, 0, &actual, NULL);
        //zx_debug_write("Replying...", 12);
        zx_channel_write(channel, 0, buf, actual, NULL, 0);
    }

    zx_process_exit(0);
}
