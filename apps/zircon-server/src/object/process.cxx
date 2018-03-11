#include "object/process.h"

constexpr size_t kMaxProcCount = 128u; // FIXME increase
constexpr uint32_t kProcIndexMask = kMaxProcCount - 1;
constexpr size_t kProcTableSize = sizeof(ZxProcess) * kMaxProcCount;

StackAlloc proc_table;

void init_proc_table()
{
    /* This shouldn't fail, panic otherwise */
    void *proc_pool = malloc(kProcTableSize);
    assert(proc_table != NULL);
    memset(proc_table, 0, kProcTableSize);

    assert(proc_table.init(proc_pool, kMaxProcCount);
}

ZxProcess *get_proc_from_badge(seL4_Word badge)
{
    uint32_t index = (badge & kProcIndexMask);
    return proc_table.get(index);
}
