#include "object/process.h"
#include "zxcpp/stackalloc.h"

constexpr size_t kMaxProcCount = 512u;
constexpr uint32_t kProcIndexMask = kMaxProcCount - 1;
constexpr size_t kProcTableSize = sizeof(ZxProcess) * kMaxProcCount;
constexpr size_t kProcTableNumPages = (kProcTableSize + BIT(seL4_PageBits) - 1) / BIT(seL4_PageBits);

StackAlloc<ZxProcess> proc_table;

/*
 *  ZxProcess non-member functions
 */
void init_proc_table(vspace_t *vspace)
{
    /* Configure pages for proc pool */
    vspace_new_pages_config_t config;
    default_vspace_new_pages_config(kProcTableNumPages, seL4_PageBits, &config);
    vspace_new_pages_config_set_vaddr((void *)ZX_PROCESS_TABLE_START, &config);

    /* Allocate proc pool */
    void *proc_pool = vspace_new_pages_with_config(vspace, &config, seL4_AllRights);
    assert(proc_pool != NULL);
    memset(proc_pool, 0, kProcTableSize);

    /* Create alloc object */
    assert(proc_table.init(proc_pool, kMaxProcCount));

    dprintf(ALWAYS, "Proc table created at %p, %lu pages\n", proc_pool, kProcTableNumPages);
    dprintf(ALWAYS, "End of proc table at %p\n", (void *)(((uintptr_t)proc_pool)
            + (kProcTableNumPages * BIT(seL4_PageBits))));
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

/*
 *  ZxProcess member functions
 */
bool ZxProcess::init(vka_t *vka, vspace_t *server_vspace)
{
    (void)vka;
    (void)server_vspace;
    return true;
}

void ZxProcess::destroy(vka_t *vka)
{
    (void)vka;
}
