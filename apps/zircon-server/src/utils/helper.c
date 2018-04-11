/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/* Include Kconfig variables. */
#include <autoconf.h>

#include <sel4/sel4.h>
#include <elf/elf.h>
#include <sel4utils/elf.h>

/* Wrappers for functions CXX doesn't like. */

uint16_t get_num_elf_headers(void *elf_file)
{
    return elf_getNumProgramHeaders(elf_file);
}

uint32_t get_elf_header_type(void *elf_file, uint16_t i)
{
    return elf_getProgramHeaderType(elf_file, i);
}

uint64_t get_elf_entry_point(void *elf_file)
{
    return elf_getEntryPoint(elf_file);
}

void get_elf_file_info(char *elf_file, uint16_t i,
        char **source_addr, uint64_t *file_size,
        uint64_t *segment_size, uint64_t *vaddr, uint64_t *flags)
{
    *source_addr = elf_file + elf_getProgramHeaderOffset(elf_file, i);
    *file_size = elf_getProgramHeaderFileSize(elf_file, i);
    *segment_size = elf_getProgramHeaderMemorySize(elf_file, i);
    *vaddr = elf_getProgramHeaderVaddr(elf_file, i);
    *flags = elf_getProgramHeaderFlags(elf_file, i);
}
