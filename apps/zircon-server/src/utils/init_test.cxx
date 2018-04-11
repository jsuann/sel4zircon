#include "server.h"
#include "object/handle.h"
#include "object/process.h"
#include "object/vmar.h"
#include "utils/elf.h"

namespace InitTestCxx {

constexpr uintptr_t TestStackBaseAddr = 0x30000000;
constexpr size_t TestStackNumPages = 15;

} /* namespace InitTestCxx */

extern "C" void init_zircon_test(void);

void init_zircon_test(void)
{
    using namespace InitTestCxx;

    /* Base objects for zircon test */
    ZxVmar *test_vmar;
    ZxProcess *test_proc;
    ZxThread *test_thread;
    ZxVmo **elf_vmos;
    ZxVmo *stack_vmo;

    /* Create a root vmar */
    test_vmar = allocate_object<ZxVmar>();
    assert(test_vmar != NULL);

    /* Create a process */
    test_proc = allocate_object<ZxProcess>(test_vmar);
    assert(test_proc != NULL);
    test_proc->set_name("zircon-test");
    assert(test_proc->init());

    /* Create a thread */
    uint32_t thrd_index;
    uint32_t proc_index = test_proc->get_proc_index();
    assert(test_proc->alloc_thread_index(thrd_index));
    test_thread = allocate_object<ZxThread>(proc_index, thrd_index);
    assert(test_thread != NULL);
    test_thread->set_name("zircon-test-thread");
    assert(test_thread->init());
    test_proc->add_thread(test_thread);

    /* Create VMOs for elf segments */
    int num_vmos;
    uintptr_t entry = load_elf_segments(test_proc, "zircon-test", num_vmos, elf_vmos);
    assert(entry != 0);
    assert(elf_vmos != NULL);
    for (int i = 0; i < num_vmos; ++i) {
        assert(elf_vmos[i] != NULL);
    }

    /* Create stack VMO */
    stack_vmo = allocate_object<ZxVmo>(TestStackNumPages);
    assert(stack_vmo->init());
    VmoMapping *stack_map = stack_vmo->create_mapping(TestStackBaseAddr, test_vmar,
            ZX_VM_FLAG_PERM_READ | ZX_VM_FLAG_PERM_WRITE);
    assert(stack_map != NULL);
    assert(stack_vmo->commit_all_pages(stack_map));

    /* Start test process */
    assert(spawn_zircon_proc(test_thread, stack_vmo, stack_map->get_base(),
            "zircon-test", entry));

    /* TODO: add vmo handles? not strictly required since vmap counts as ref */

    /* Get handles to test objects */
    Handle *vmar_handle = test_vmar->create_handle(ZX_DEFAULT_VMAR_RIGHTS);
    Handle *proc_handle = test_proc->create_handle(ZX_DEFAULT_PROCESS_RIGHTS);
    Handle *thrd_handle = test_thread->create_handle(ZX_DEFAULT_THREAD_RIGHTS);

    /* Add handles to process */
    test_proc->add_handle(vmar_handle);
    test_proc->add_handle(proc_handle);
    test_proc->add_handle(thrd_handle);
    test_proc->print_handles();

    /* Get user handle values */
    zx_handle_t vmar_uval = test_proc->get_handle_user_val(vmar_handle);
    zx_handle_t proc_uval = test_proc->get_handle_user_val(proc_handle);
    zx_handle_t thrd_uval = test_proc->get_handle_user_val(thrd_handle);

    /* Send handles to zircon test */
    dprintf(SPEW, "Sending test data to zircon test!\n");
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 3);
    seL4_SetMR(0, vmar_uval);
    seL4_SetMR(1, proc_uval);
    seL4_SetMR(2, thrd_uval);
    seL4_Send(get_server_ep(), tag);
}
