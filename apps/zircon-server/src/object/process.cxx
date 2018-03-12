#include "object/process.h"
#include "zxcpp/stackalloc.h"

constexpr size_t kMaxProcCount = 128u; // FIXME increase
constexpr uint32_t kProcIndexMask = kMaxProcCount - 1;
constexpr size_t kProcTableSize = sizeof(ZxProcess) * kMaxProcCount;

StackAlloc<ZxProcess> proc_table;

void init_proc_table()
{
    /* This shouldn't fail, panic otherwise */
    void *proc_pool = malloc(kProcTableSize);
    assert(proc_pool != NULL);
    assert(proc_table.init(proc_pool, kMaxProcCount));
}

ZxProcess *get_proc_from_badge(uint64_t badge)
{
    uint32_t index = (badge & kProcIndexMask);
    return proc_table.get(index);
}

template <>
ZxProcess *allocate_object<ZxProcess>(ZxVmar *root_vmar)
{
    uint32_t index;
    seL4_Word badge_val;
    if (!proc_table.alloc(index)) {
        return NULL;
    }
    /* TODO some other ident for procs? */
    badge_val = index;
    void *p = (void *)proc_table.get(index);
    return new (p) ZxProcess(root_vmar, badge_val);
}

template <>
void free_object<ZxProcess>(ZxProcess *obj)
{
    uint32_t index = (obj->get_badge_val() & kProcIndexMask);
    proc_table.free(index);
}
