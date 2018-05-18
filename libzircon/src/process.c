#include <assert.h>
#include <zircon/syscalls.h>
#include <zircon/process.h>
#include <zircon/processargs.h>

/* Startup handles stored here */
zx_handle_t startup_handles[SZX_NUM_HANDLES] = {0};

void zx_init_startup_handles(char *argv[])
{
    /* Channel handle is located at argv[0] */
    zx_handle_t channel = *((zx_handle_t *)argv[0]);

    /* Read init handles from channel */
    uint32_t actual_handles;
    zx_status_t err;
    err = zx_channel_read(channel, 0, NULL, &startup_handles[0],
            0, SZX_NUM_HANDLES, NULL, &actual_handles);
    if (err != ZX_OK) {
        /* We assume this is because handles
           have already been read */
        return;
    }

    /* We may not always get the resource handle */
    assert(actual_handles >= (SZX_NUM_HANDLES - 1));

    /* Close the channel */
    zx_handle_close(channel);
}

#define SZX_PROC_SELF       0x0
#define SZX_THREAD_SELF     0x1
#define SZX_JOB_DEFAULT     0x2
#define SZX_VMAR_ROOT       0x3
#define SZX_RESOURCE_ROOT   0x4

zx_handle_t zx_thread_self(void)
{
    return startup_handles[SZX_THREAD_SELF];
}

zx_handle_t zx_process_self(void)
{
    return startup_handles[SZX_PROC_SELF];
}

zx_handle_t zx_vmar_root_self(void)
{
    return startup_handles[SZX_VMAR_ROOT];
}

zx_handle_t zx_job_default(void)
{
    return startup_handles[SZX_JOB_DEFAULT];
}

zx_handle_t zx_resource_root(void)
{
    return startup_handles[SZX_RESOURCE_ROOT];
}

zx_handle_t zx_get_startup_handle(uint32_t hnd_info)
{
    return ZX_HANDLE_INVALID;
}
