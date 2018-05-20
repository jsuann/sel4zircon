#include "utils/page_alloc.h"
#include "addrspace.h"

namespace PageAllocCxx {

constexpr size_t kNumPageBuf = 4096;
constexpr size_t kPageSize = 1 << seL4_PageBits;

/* Allocate pages with adjacent guard pages */
constexpr size_t kBufBlockSize = (1 << seL4_PageBits) * 2;

struct BufBlock {
    char mem[kBufBlockSize];
};

StackAlloc<BufBlock> page_buf_table;

uint32_t get_page_index(void *page)
{
    return ((uintptr_t)page - ZX_PAGE_BUF_START) / sizeof(BufBlock);
}

} /* namespace MBufCxx */

void init_page_alloc(vka_t *vka)
{
    using namespace PageAllocCxx;

    /* Alloc the alloc table */
    dprintf(INFO, "Creating page allocator at %p, num pages: %lu\n",
            (void *)ZX_PAGE_BUF_START, kNumPageBuf);
    assert(page_buf_table.init((BufBlock *)ZX_PAGE_BUF_START, kNumPageBuf));

    /* Alloc pages at each block */
    for (uint32_t i = 0; i < kNumPageBuf; ++i) {
        vka_object_t frame;
        uintptr_t addr = ZX_PAGE_BUF_START + (i * sizeof(BufBlock));
        /* Alloc a frame */
        int err = vka_alloc_frame(vka, seL4_PageBits, &frame);
        assert(!err);
        /* Map the frame. Won't be unmapped so leak everything. */
        err = sel4utils_map_page_leaky(vka, seL4_CapInitThreadVSpace,
                frame.cptr, (void *)addr, seL4_AllRights, 1);
        assert(!err);
    }

    dprintf(INFO, "End of page alloc at %p\n",
            (void *)(ZX_PAGE_BUF_START + (sizeof(BufBlock) * kNumPageBuf)));
}

uint32_t get_num_page_avail()
{
    return PageAllocCxx::page_buf_table.num_avail();
}

void *page_alloc()
{
    using namespace PageAllocCxx;

    uint32_t index;
    if(!page_buf_table.alloc(index)) {
        dprintf(CRITICAL, "Ran out of page bufs!\n");
        return NULL;
    }
    return (void *)page_buf_table.get(index);
}

void *page_alloc_zero()
{
    using namespace PageAllocCxx;

    void *p = page_alloc();
    if (p != NULL) {
        memset(p, 0, kPageSize);
    }
    dprintf(SPEW, "Allocd page at %p\n", p);
    return p;
}

void page_free(void *page)
{
    using namespace PageAllocCxx;

    dprintf(SPEW, "Freeing page at %p\n", page);
    uint32_t index = get_page_index(page);
    page_buf_table.free(index);
}
