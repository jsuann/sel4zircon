#include <autoconf.h>

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "addrspace.h"
#include "zxcpp/stackalloc.h"

constexpr size_t kNumVmoBlocks = (ZX_VMO_SERVER_MAP_END - ZX_VMO_SERVER_MAP_START) / ZX_VMO_SERVER_MAP_SIZE;

/* define a struct with the size of a VMO block to use with Stack Alloc */
struct VmoBlock {
    char mem[ZX_VMO_SERVER_MAP_SIZE];
};

StackAlloc<VmoBlock> vtable;

void init_vmo_kmap(void)
{
    dprintf(INFO, "Creating vtable for VMO kmap, num blocks: %lu\n", kNumVmoBlocks);
    assert(vtable.init((VmoBlock *)ZX_VMO_SERVER_MAP_START, kNumVmoBlocks));
}

uintptr_t alloc_vmo_kmap(void)
{
    uint32_t index;
    if (!vtable.alloc(index)) {
        return 0;
    }
    dprintf(SPEW, "Vtable allocated VMO kmap region at %p\n", (void *)vtable.get(index));
    return (uintptr_t)vtable.get(index);
}

void free_vmo_kmap(uintptr_t kmap)
{
    /* Get index from kmap */
    uint32_t index = (kmap - ZX_VMO_SERVER_MAP_START) / ZX_VMO_SERVER_MAP_SIZE;
    vtable.free(index);
}
