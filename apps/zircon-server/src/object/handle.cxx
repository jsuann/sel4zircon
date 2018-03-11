#include "object/handle.h"

constexpr size_t kMaxHandleCount = 16 * 1024u;

constexpr uint32_t kHandleIndexMask = kMaxHandleCount - 1;

constexpr uint32_t kHandleGenerationMask = ~kHandleIndexMask & ~(3 << 30);

/* Must be floor(log2(kMaxHandleCount)) */
//constexpr uint32_t kHandleGenerationShift = 14;
constexpr uint32_t kHandleGenerationShift = 24;

StackAlloc handle_table;

uint32_t get_new_base_value(void *p)
{
    /* For now, just cast addr to val */
    uint64_t val = (uint64_t)p;
    uint32_t base_value = val;
    dprintf(SPEW, "Handle val sanity check: %lx, %p & %x should match!\n", val, p, base_value);
    return base_value;
}

/*
Handle *base_value_to_addr()
{

}
*/
