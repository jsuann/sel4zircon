#include "test/elf.h"
#include "server.h"
#include "addrspace.h"
#include "object/vmo.h"

extern "C" {
#include <sel4/sel4.h>
//#include <sel4utils/elf.h>
//#include <elf.h>
#include <elf/elf.h>
#include <cpio/cpio.h>
#include "debug.h"
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
uintptr_t load_elf_segments(ZxProcess *proc, const char *image_name)
{
    using namespace ElfCxx;

    unsigned long elf_size;
    char *elf_file = (char *)cpio_get_file(_cpio_archive, image_name, &elf_size);
    if (elf_file == NULL) {
        dprintf(INFO, "Failed to get elf file for %s\n", image_name);
        return 0;
    }

    ZxVmar *root_vmar = proc->get_root_vmar();
    uint16_t num_headers = elf_getNumProgramHeaders(elf_file);

    uint64_t entry_point = elf_getEntryPoint(elf_file);
    assert(entry_point != 0);

    for (uint16_t i = 0; i < num_headers; ++i) {
        char *source_addr;
        unsigned long flags, file_size, segment_size, vaddr;

        /* Skip non-loadable segments */
        if (elf_getProgramHeaderType(elf_file, i) == PT_LOAD) {
            /* Fetch segment info */
            source_addr = elf_file + elf_getProgramHeaderOffset(elf_file, i);
            file_size = elf_getProgramHeaderFileSize(elf_file, i);
            segment_size = elf_getProgramHeaderMemorySize(elf_file, i);
            vaddr = elf_getProgramHeaderVaddr(elf_file, i);
            flags = elf_getProgramHeaderFlags(elf_file, i);

            /* Make a VMO for this segment */
            ZxVmo *vmo = create_elf_vmo(root_vmar, vaddr, segment_size, flags);
            if (vmo == NULL) {
                dprintf(INFO, "Failed to create vmo for segment %u\n", i);
                return 0;
            }

            /* Copy segment to vmo */
            uint64_t offset = vaddr - (vaddr & kPageMask);
            vmo->write(offset, file_size, (uintptr_t)source_addr);
        }
    }
    return entry_point;
}
