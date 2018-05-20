#include "utils/elf.h"
#include "server.h"
#include "addrspace.h"

extern "C" {
#include <sel4/sel4.h>
#include <cpio/cpio.h>
#include <elf.h>
#include <sel4utils/elf.h>
#include <sel4utils/helpers.h>
#include "debug.h"
}

namespace ElfCxx {

/* This will get linked */
extern "C" char _cpio_archive[];

/* Wrappers for elf parsing functions CXX doesn't like */
extern "C" {
uint16_t get_num_elf_headers(void *elf_file);
uint32_t get_elf_header_type(void *elf_file, uint16_t i);
uint64_t get_elf_entry_point(void *elf_file);
void get_elf_file_info(char *elf_file, uint16_t i,
        char **source_addr, uint64_t *file_size,
        uint64_t *segment_size, uint64_t *vaddr, uint64_t *flags);
}

/* constants */
constexpr uint64_t kPageSize = (1 << seL4_PageBits);
constexpr uint64_t kPageMask = ~(kPageSize - 1);
constexpr uint64_t kStackAlign = 16;
constexpr uint64_t kStackAlignMask = ~(kStackAlign - 1);


/* Create a vmo that will contain an elf segment */
ZxVmo *create_elf_vmo(ZxVmar *vmar, unsigned long vaddr,
        unsigned long segment_size, unsigned long permissions)
{
    ZxVmo *vmo = NULL;
    VmoMapping *vmap = NULL;

    /* Vmo start addr & size need to be rounded to page boundaries */
    uintptr_t vmo_start = (vaddr & kPageMask);
    size_t vmo_size = ((vaddr + segment_size + kPageSize - 1) & kPageMask) - vmo_start;
    assert(vmo_size < ZX_VMO_SERVER_MAP_SIZE);
    dprintf(INFO, "Creating elf vmo, size %lu num pages %lu\n", vmo_size, vmo_size/kPageSize);

    /* Create the vmo */
    size_t num_pages = vmo_size / kPageSize;
    vmo = allocate_object<ZxVmo>(num_pages);
    vmo->init();

    /* Convert elf rights to zircon vmo flags */
    uint32_t vmo_flags = 0;
    if (permissions & PF_R || permissions & PF_X) {
        vmo_flags |= ZX_VM_FLAG_PERM_READ;
    }
    if (permissions & PF_W) {
        vmo_flags |= ZX_VM_FLAG_PERM_WRITE;
    }

    /* Create a vmap in supplied vmar */
    vmap = vmo->create_mapping(vmo_start, 0, vmo_size, vmar, vmo_flags, 0);

    /* Back vmo with pages */
    vmo->commit_all_pages(vmap);

    return vmo;
}

/* Write to stack vmo and update stack pointer */
void write_to_stack(ZxVmo *stack_vmo, uintptr_t stack_base,
        uintptr_t *stack_ptr, void *buf, size_t len)
{
    /* Calculate offset to write to vmo */
    uintptr_t new_stack_ptr = (*stack_ptr) - len;
    uint64_t offset = new_stack_ptr - stack_base;
    assert(stack_vmo->get_size() > offset);

    /* Write to stack (should already be mapped in!) */
    stack_vmo->write(offset, len, buf);

    /* Update stack ptr */
    *stack_ptr = new_stack_ptr;
}

/* Write a constant value to stack vmo */
void write_constant_to_stack(ZxVmo *stack_vmo, uintptr_t stack_base,
        uintptr_t *stack_ptr, seL4_Word value)
{
    write_to_stack(stack_vmo, stack_base, stack_ptr, (void *)&value, sizeof(value));
}

} /* namespace ElfCxx */

/* Just get the elf file from the cpio archive */
char *get_elf_file(const char *image_name, unsigned long *elf_size)
{
    using namespace ElfCxx;
    return (char *)cpio_get_file(_cpio_archive, image_name, elf_size);
}

/* creates required VMOs and loads elf segments into them */
uintptr_t load_elf_segments(ZxProcess *proc, const char *image_name,
        int &num_vmos, ZxVmo **&vmos)
{
    using namespace ElfCxx;

    num_vmos = 0;
    vmos = NULL;

    unsigned long elf_size;
    char *elf_file = (char *)cpio_get_file(_cpio_archive, image_name, &elf_size);
    if (elf_file == NULL) {
        dprintf(INFO, "Failed to get elf file for %s\n", image_name);
        return 0;
    }

    ZxVmar *root_vmar = proc->get_root_vmar();
    uint16_t num_headers = get_num_elf_headers(elf_file);

    uint64_t entry_point = get_elf_entry_point(elf_file);
    assert(entry_point != 0);

    /* Figure out how many VMOs we need, make array */
    for (uint16_t i = 0; i < num_headers; ++i) {
        if (get_elf_header_type(elf_file, i) == PT_LOAD) {
            ++num_vmos;
        }
    }
    vmos = (ZxVmo **)calloc(num_vmos, sizeof(ZxVmo*));
    if (vmos == NULL) {
        return 0;
    }

    int vmo_index = 0;
    for (uint16_t i = 0; i < num_headers; ++i) {
        char *source_addr;
        unsigned long flags, file_size, segment_size, vaddr;

        /* Skip non-loadable segments */
        if (get_elf_header_type(elf_file, i) == PT_LOAD) {
            /* Fetch segment info */
            get_elf_file_info(elf_file, i, &source_addr, &file_size,
                    &segment_size, &vaddr, &flags);

            dprintf(INFO, "elf info: %p %lx %lx %lx %lu\n", source_addr,
                    file_size, segment_size, vaddr, flags);
            /* Make a VMO for this segment */
            ZxVmo *elf_vmo = create_elf_vmo(root_vmar, vaddr, segment_size, flags);
            if (elf_vmo == NULL) {
                dprintf(INFO, "Failed to create vmo for segment %u\n", i);
                return 0;
            }

            /* Copy segment to vmo */
            uint64_t offset = vaddr - (vaddr & kPageMask);
            elf_vmo->write(offset, file_size, source_addr);

            /* Store vmo ptr */
            vmos[vmo_index] = elf_vmo;
            ++vmo_index;
        }
    }
    return entry_point;
}

/* Spawns zircon process with vsyscall. Requires elf & stack vmos ready. */
bool spawn_zircon_proc(ZxThread *thrd, ZxVmo *stack_vmo, uintptr_t stack_base,
        const char *image_name, uintptr_t entry, zx_handle_t channel_handle)
{
    using namespace ElfCxx;

    /* Get elf info */
    uintptr_t vsyscall = sel4utils_elf_get_vsyscall(image_name);
    uint32_t num_phdrs = sel4utils_elf_num_phdrs(image_name);
    Elf_Phdr *phdrs = (Elf_Phdr *)calloc(num_phdrs, sizeof(Elf_Phdr));
    if (phdrs == NULL) {
        return false;
    }
    sel4utils_elf_read_phdrs(image_name, num_phdrs, phdrs);

    /* Get stack pointer */
    uintptr_t stack_ptr = (stack_base + stack_vmo->get_size()) - sizeof(seL4_Word);

    /* Copy elf headers */
    write_to_stack(stack_vmo, stack_base, &stack_ptr, phdrs, num_phdrs * sizeof(Elf_Phdr));
    uintptr_t at_phdr = stack_ptr;

    /* Init aux vectors */
    int auxc = 5;
    Elf_auxv_t auxv[5];
    auxv[0].a_type = AT_PAGESZ;
    auxv[0].a_un.a_val = kPageSize;
    auxv[1].a_type = AT_PHDR;
    auxv[1].a_un.a_val = at_phdr;
    auxv[2].a_type = AT_PHNUM;
    auxv[2].a_un.a_val = num_phdrs;
    auxv[3].a_type = AT_PHENT;
    auxv[3].a_un.a_val = sizeof(Elf_Phdr);
    auxv[4].a_type = AT_SYSINFO;
    auxv[4].a_un.a_val = vsyscall;

    /* Write the channel handle to the stack */
    write_constant_to_stack(stack_vmo, stack_base, &stack_ptr, 0);
    write_constant_to_stack(stack_vmo, stack_base, &stack_ptr, channel_handle);
    uintptr_t handle_argv = stack_ptr;

    /* Ensure alignment of stack ptr (double word alignment) */
    size_t to_push = (6 * sizeof(seL4_Word)) + (sizeof(auxv[0]) * auxc);
    uintptr_t rounded_stack_ptr = (stack_ptr - to_push) & kStackAlignMask;
    stack_ptr = rounded_stack_ptr + to_push;

    /* Write aux */
    write_constant_to_stack(stack_vmo, stack_base, &stack_ptr, 0);
    write_constant_to_stack(stack_vmo, stack_base, &stack_ptr, 0);
    write_to_stack(stack_vmo, stack_base, &stack_ptr, auxv, sizeof(auxv[0]) * auxc);

    /* Write empty env */
    write_constant_to_stack(stack_vmo, stack_base, &stack_ptr, 0);

    /* Write args (null argv, handle_argv, argc) */
    write_constant_to_stack(stack_vmo, stack_base, &stack_ptr, 0);
    write_constant_to_stack(stack_vmo, stack_base, &stack_ptr, handle_argv);
    write_constant_to_stack(stack_vmo, stack_base, &stack_ptr, 1);

    assert(stack_ptr % kStackAlign == 0);
    dprintf(INFO, "New proc, entry %lx, stack %lx\n", entry, stack_ptr);

    /* Start thread */
    if (thrd->start_execution(entry, stack_ptr, 0, 0) != 0) {
        return false;
    }

    return true;
}
