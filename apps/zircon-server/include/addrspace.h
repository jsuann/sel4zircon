#pragma once

/* Zircon server address space allocation */
#define ZX_HANDLE_TABLE_START       0x01000000ull
#define ZX_PROCESS_TABLE_START      0x04000000ull

#define ZX_VMO_SERVER_MAP_START     0x400000000ull

/* VMOs can be max 16gb */
#define ZX_VMO_SERVER_MAP_SIZE      0x400000000ull

/* End of low vaddr space */
#define ZX_VMO_SERVER_MAP_END       0x800000000000ull

/* Functions for allocating VMO mappings on server */
void init_vmo_kmap(void);
uintptr_t alloc_vmo_kmap(void);
void free_vmo_kmap(uintptr_t kmap);
