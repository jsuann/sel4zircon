#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <zircon/syscalls.h>
#include <zircon/process.h>
#include <sel4zircon/debug.h>
#include <sel4zircon/elf.h>

#define ELF_VMO_SIZE    0x200000
#define PAGE_MASK       ~((1 << seL4_PageBits) - 1)
#define STACK_SIZE      (20 * (1 << seL4_PageBits))

zx_status_t run_zircon_app(const char *filename, zx_handle_t *process,
        zx_handle_t *channel, uint64_t arg)
{
    zx_status_t err;
    uint64_t mapped_addr = 0;
    uint64_t filesize = 0;
    zx_handle_t elf_vmo = ZX_HANDLE_INVALID;
    zx_handle_t stack_vmo = ZX_HANDLE_INVALID;
    *process = ZX_HANDLE_INVALID;
    *channel = ZX_HANDLE_INVALID;
    zx_handle_t other_channel = ZX_HANDLE_INVALID;

    /* Create a vmo to load the elf file in */
    err = zx_vmo_create(ELF_VMO_SIZE, 0, &elf_vmo);
    if (err) {
        printf("elf vmo create failed.\n");
        goto zx_app_cleanup;
    }

    /* Get the elf file */
    uint32_t name_len = strlen(filename) + 1;
    err = zx_get_elf_vmo(zx_resource_root(), elf_vmo,
            filename, name_len, &filesize);
    if (err) {
        printf("get elf vmo failed.\n");
        goto zx_app_cleanup;
    }

    /* Map vmo anywhere in our address space */
    err = zx_vmar_map(zx_vmar_root_self(), 0, elf_vmo,
            0, filesize, ZX_VM_FLAG_PERM_READ, &mapped_addr);
    if (err) {
        printf("elf vmo map failed.\n");
        goto zx_app_cleanup;
    }

    /* Get elf file info */
    char *elf_file = (char *)mapped_addr;
    uint16_t num_headers = elf_getNumProgramHeaders(elf_file);
    uint64_t entry = elf_getEntryPoint(elf_file);
    /* Check entry lies within root vmar */
    if (entry < 0x1000000ul) {
        printf("Invalid entry point!\n");
        err = -1;
        goto zx_app_cleanup;
    }

    /* Elf file looks ok, so prepare a process for loading */
    zx_handle_t vmar;
    err = zx_process_create(zx_job_default(), filename, name_len,
            0, process, &vmar);
    if (err) {
        printf("proc create failed.\n");
        goto zx_app_cleanup;
    }

    /* Prepare the thread */
    zx_handle_t thread;
    err = zx_thread_create(*process, filename, name_len, 0, &thread);
    if (err) {
        printf("thread create failed.\n");
        goto zx_app_cleanup;
    }

    /* For each loadable elf segment, create a vmo, write contents,
       and map in vmar */
    for (uint16_t i = 0; i < num_headers; ++i) {
        char *source_addr;
        unsigned long flags, file_size, segment_size, vaddr;
        if (elf_getProgramHeaderType(elf_file, i) == PT_LOAD) {
            /* Get info */
            source_addr = elf_file + elf_getProgramHeaderOffset(elf_file, i);
            file_size = elf_getProgramHeaderFileSize(elf_file, i);
            segment_size = elf_getProgramHeaderMemorySize(elf_file, i);
            vaddr = elf_getProgramHeaderVaddr(elf_file, i);
            flags = elf_getProgramHeaderFlags(elf_file, i);
            /* Page align various bits */
            uint64_t offset = vaddr - (vaddr & PAGE_MASK);
            segment_size = (segment_size + offset + (1 << seL4_PageBits)) & PAGE_MASK;
            /* Make a vmo */
            zx_handle_t segment_vmo;
            err = zx_vmo_create(segment_size, 0, &segment_vmo);
            if (err) {
                printf("segment vmo create failed.\n");
                goto zx_app_cleanup;
            }
            /* Write contents to vmo */
            size_t actual;
            err = zx_vmo_write(segment_vmo, source_addr, offset, file_size, &actual);
            if (err) {
                printf("segment vmo write failed.\n");
                zx_handle_close(segment_vmo);
                goto zx_app_cleanup;
            }
            /* Convert permission flags into map flags */
            uint32_t map_flags = ZX_VM_FLAG_SPECIFIC;
            map_flags |= (flags & PF_R) ? ZX_VM_FLAG_PERM_READ : 0;
            map_flags |= (flags & PF_W) ? ZX_VM_FLAG_PERM_WRITE : 0;
            map_flags |= (flags & PF_X) ? ZX_VM_FLAG_PERM_EXECUTE : 0;
            /* Map into vmar */
            vaddr &= PAGE_MASK;
            err = zx_vmar_map(vmar, vaddr, segment_vmo, 0, segment_size, map_flags, &vaddr);
            if (err) {
                printf("segment vmo map failed. %d\n", err);
                zx_handle_close(segment_vmo);
                goto zx_app_cleanup;
            }
        }
    }

    /* Create a stack vmo, and map in vmar */
    uint64_t stack_base;
    err = zx_vmo_create(STACK_SIZE, 0, &stack_vmo);
    if (err) {
        printf("stack vmo create failed.\n");
        goto zx_app_cleanup;
    }
    err = zx_vmar_map(vmar, 0, stack_vmo, 0, STACK_SIZE,
            ZX_VM_FLAG_PERM_READ | ZX_VM_FLAG_PERM_WRITE, &stack_base);
    if (err) {
        printf("stack vmo map failed.\n");
        goto zx_app_cleanup;
    }

    /* Create channel to talk to other proc */
    err = zx_channel_create(0, channel, &other_channel);
    if (err) {
        printf("channel create failed.\n");
        goto zx_app_cleanup;
    }

    /* Start the process */
    uintptr_t sp = (stack_base + STACK_SIZE) & -16;
    err = zx_process_start(*process, thread, entry, sp, other_channel, arg);
    if (err) {
        printf("process start failed.\n");
        goto zx_app_cleanup;
    }

    /* Process started successfully. Clean up the elf vmo */
    zx_vmar_unmap(zx_vmar_root_self(), mapped_addr, filesize);
    zx_handle_close(elf_vmo);
    return ZX_OK;

zx_app_cleanup:
    zx_handle_close(elf_vmo);
    zx_handle_close(stack_vmo);
    zx_handle_close(vmar);
    zx_handle_close(*process);
    zx_handle_close(*channel);
    zx_handle_close(other_channel);

    if (mapped_addr != 0) {
        zx_vmar_unmap(zx_vmar_root_self(), mapped_addr, filesize);
    }

    fflush(stdout);
    return err;
}
