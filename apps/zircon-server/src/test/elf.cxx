#include "test/elf.h"
#include "server.h"
#include "addrspace.h"

extern "C" {
#include <sel4/sel4.h>
#include <cpio/cpio.h>
#include <elf.h>
#include <sel4utils/elf.h>
#include "debug.h"
}

extern "C" {
uint16_t get_num_elf_headers(void *elf_file);
uint32_t get_elf_header_type(void *elf_file, uint16_t i);
uint64_t get_elf_entry_point(void *elf_file);
void get_elf_file_info(char *elf_file, uint16_t i,
        char **source_addr, uint64_t *file_size,
        uint64_t *segment_size, uint64_t *vaddr, uint64_t *flags);
}

namespace ElfCxx {

constexpr uint64_t kPageSize = (1 << seL4_PageBits);
constexpr uint64_t kPageMask = ~(kPageSize - 1);

ZxVmo *create_elf_vmo(ZxVmar *vmar, unsigned long vaddr,
        unsigned long segment_size, unsigned long permissions)
{
    ZxVmo *vmo = NULL;
    VmoMapping *vmap = NULL;

    /* Vmo start addr & size need to be rounded to page boundaries */
    uintptr_t vmo_start = (vaddr & kPageMask);
    size_t vmo_size = ((vaddr + segment_size + kPageSize - 1) & kPageMask) - vmo_start;
    assert(vmo_size < ZX_VMO_SERVER_MAP_SIZE);

    /* Create the vmo */
    size_t num_pages = vmo_size / kPageSize;
    vmo = allocate_object<ZxVmo>(num_pages); // TODO error
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
    vmap = vmo->create_mapping(vmo_start, vmar, vmo_flags);

    /* Back vmo with pages */
    vmo->commit_all_pages(vmap);

    return vmo;
}

} /* namespace ElfCxx */

extern "C" char _cpio_archive[];

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
            elf_vmo->write(offset, file_size, (uintptr_t)source_addr);

            /* Store vmo ptr */
            vmos[vmo_index] = elf_vmo;
            ++vmo_index;
        }
    }
    return entry_point;
}
