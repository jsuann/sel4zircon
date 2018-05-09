#include "object/handle.h"
#include "zxcpp/stackalloc.h"
#include "addrspace.h"

namespace HandleCxx {

constexpr size_t kMaxHandleCount = 64 * 1024u;

constexpr uint32_t kHandleIndexMask = kMaxHandleCount - 1;

constexpr uint32_t kHandleGenerationMask = ~kHandleIndexMask & ~(3 << 30);

/* Max 1 << 20 handles */
constexpr uint32_t kHandleGenerationShift = 20;

constexpr size_t kHandleTableSize = sizeof(Handle) * kMaxHandleCount;
constexpr size_t kHandleTableNumPages = (kHandleTableSize + BIT(seL4_PageBits) - 1) / BIT(seL4_PageBits);

StackAlloc<Handle> handle_table;

uint32_t get_new_base_value(void *p, uint32_t index)
{
    /* Check if this handle slot has been used before */
    Handle *h = (Handle *)p;
    uint32_t old_gen = 0;
    if (h->get_value() != 0) {
        dprintf(SPEW, "Handle at %p has been used before! Bumping gen.\n", p);
        old_gen = (h->get_value() & kHandleGenerationMask) >> kHandleGenerationShift;
    }
    uint32_t new_gen = (((old_gen + 1) << kHandleGenerationShift) & kHandleGenerationMask);
    return (index | new_gen);
}

} /* namespace HandleCxx */


void init_handle_table(vspace_t *vspace)
{
    using namespace HandleCxx;

    /* Configure pages for handle pool */
    vspace_new_pages_config_t config;
    default_vspace_new_pages_config(kHandleTableNumPages, seL4_PageBits, &config);
    vspace_new_pages_config_set_vaddr((void *)ZX_HANDLE_TABLE_START, &config);

    /* Allocate handle pool */
    void *handle_pool = vspace_new_pages_with_config(vspace, &config, seL4_AllRights);
    assert(handle_pool != NULL);

    /* Create alloc object */
    assert(handle_table.init(handle_pool, kMaxHandleCount));

    dprintf(ALWAYS, "Handle table created at %p, %lu pages\n", handle_pool, kHandleTableNumPages);
    dprintf(ALWAYS, "End of handle table at %p\n", (void *)(((uintptr_t)handle_pool)
            + (kHandleTableNumPages * BIT(seL4_PageBits))));
}

Handle *allocate_handle(ZxObject *obj, zx_rights_t rights)
{
    using namespace HandleCxx;

    uint32_t index;
    if (!handle_table.alloc(index)) {
        return NULL;
    }
    void *p = (void *)handle_table.get(index);
    uint32_t base_value = get_new_base_value(p, index);
    return new (p) Handle(obj, rights, base_value);
}

void free_handle(Handle *h)
{
    using namespace HandleCxx;

    uint32_t index = (h->get_value() & kHandleIndexMask);
    handle_table.free(index);
}

Handle *base_value_to_addr(uint32_t base_value)
{
    using namespace HandleCxx;

    uint32_t index = (base_value & kHandleIndexMask);
    if (unlikely(!handle_table.is_alloc(index))) {
        return NULL;
    }
    Handle *h = handle_table.get(index);
    /* Sanity check the base value */
    return likely(h->get_value() == base_value) ? h : NULL;
}
