#pragma once

/* Zircon server address space allocation */
#define ZX_HANDLE_TABLE_START       0x01000000ul
#define ZX_PROCESS_TABLE_START      0x04000000ul

#define ZX_VMO_SERVER_MAP_START     0x400000000ul

/* VMOs can be max 16gb */
#define ZX_VMO_SERVER_MAP_SIZE      0x400000000ul

/* End of low vaddr space */
#define ZX_VMO_SERVER_MAP_END       0x800000000000ul

/* Functions for allocating VMO mappings on server */
void init_vmo_kmap(void);
uintptr_t alloc_vmo_kmap(void);
void free_vmo_kmap(uintptr_t kmap);


/* Zircon userspace address space allocation */
/* taken from zircon/kernel/arch/x86/rules.mk */
#define ZX_USER_ASPACE_BASE         0x1000000ul
#define ZX_USER_ASPACE_SIZE         0x7ffffefff000ul
/* End of user aspace is (1 << 47) - 4k */

#define ZX_USER_IPC_BUFFER_BASE     0x400000ul
