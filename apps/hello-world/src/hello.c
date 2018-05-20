#include <zircon/syscalls.h>
#include <zircon/types.h>
#include <assert.h>

#define CHANNEL_BUF_SIZE    10000

void entry(zx_handle_t channel, uintptr_t fnptr)
{
    char *hello_msg = "Hello world!";
    zx_debug_write((void *)hello_msg, 12);

    char buf[CHANNEL_BUF_SIZE] = {0};

    while (1) {
        assert(!zx_object_wait_one(channel, ZX_CHANNEL_READABLE,  ZX_TIME_INFINITE, NULL));
        assert(!zx_channel_read(channel, 0, buf, NULL, CHANNEL_BUF_SIZE, 0, NULL, NULL));
        assert(!zx_channel_write(channel, 0, buf, CHANNEL_BUF_SIZE, NULL, 0));
    }

    zx_process_exit(0);
}