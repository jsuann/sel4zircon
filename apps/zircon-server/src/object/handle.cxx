#include "object/handle.h"
#include "zxcpp/stackalloc.h"

constexpr size_t kMaxHandleCount = 8 * 1024u;

constexpr uint32_t kHandleIndexMask = kMaxHandleCount - 1;

constexpr uint32_t kHandleGenerationMask = ~kHandleIndexMask & ~(3 << 30);

/* Must be floor(log2(kMaxHandleCount)) */
//constexpr uint32_t kHandleGenerationShift = 14;
constexpr uint32_t kHandleGenerationShift = 24;

constexpr size_t kHandleTableSize = sizeof(Handle) * kMaxHandleCount;

StackAlloc<Handle> handle_table;

void init_handle_table()
{
    void *handle_pool = malloc(kHandleTableSize);
    assert(handle_pool != NULL);
    assert(handle_table.init(handle_pool, kMaxHandleCount));
}

uint32_t get_new_base_value(uint32_t index)
{
    /* For now, just use index. Will need to add gen shifting! */
    return index;
}

Handle *allocate_handle(ZxObject *obj, zx_rights_t rights)
{
    uint32_t index;
    if (!handle_table.alloc(index)) {
        return NULL;
    }
    void *p = (void *)handle_table.get(index);
    return new (p) Handle(obj, rights, get_new_base_value(index));
}

void free_handle(Handle *h)
{
    uint32_t index = (h->get_value() & kHandleIndexMask);
    handle_table.free(index);
}

Handle *base_value_to_addr(uint32_t base_value)
{
    uint32_t index = (base_value & kHandleIndexMask);
    return (handle_table.is_alloc(index)) ? handle_table.get(index) : NULL;
}
