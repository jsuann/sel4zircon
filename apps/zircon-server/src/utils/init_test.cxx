#include "server.h"
#include "object/handle.h"
#include "object/job.h"
#include "object/process.h"
#include "object/vmar.h"
#include "object/resource.h"
#include "object/channel.h"
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
    ZxChannel *ch0, *ch1;

    /* Get root objects */
    ZxResource *root_rsrc = get_root_resource();

    /* Create a root vmar */
    test_vmar = allocate_object<ZxVmar>();
    assert(test_vmar != NULL);

    /* Create a process */
    test_proc = allocate_object<ZxProcess>(test_vmar);
    assert(test_proc != NULL);
    test_proc->set_name("zircon-test");
    assert(test_proc->init());
    get_root_job()->add_process(test_proc);

    /* Create a thread */
    test_thread = allocate_object<ZxThread>();
    assert(test_thread != NULL);
    test_thread->set_name("zircon-test-thread");
    assert(test_thread->init());
    assert(test_proc->add_thread(test_thread));

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
    VmoMapping *stack_map = stack_vmo->create_mapping(TestStackBaseAddr, 0,
            stack_vmo->get_size(), test_vmar, ZX_VM_FLAG_PERM_READ | ZX_VM_FLAG_PERM_WRITE, 0);
    assert(stack_map != NULL);
    assert(stack_vmo->commit_all_pages(NULL));

    /* Create channel for sending handles. We send process ch1 */
    assert(!create_channel_pair(ch0, ch1));

    /* Get handles to test objects */
    Handle *handle_table[4];
    handle_table[0] = create_handle_default_rights(test_vmar); // TODO not just default
    handle_table[1] = create_handle_default_rights(test_proc);
    handle_table[2] = create_handle_default_rights(test_thread);
    handle_table[3] = create_handle_default_rights(root_rsrc);

    for (int i = 0; i < 4; ++i) {
        assert(handle_table[i] != NULL);
    }

    zx_handle_t channel_handle = test_proc->create_handle_get_uval(ch1);
    assert(channel_handle != ZX_HANDLE_INVALID);
    dprintf(INFO, "channel handle: %u\n", channel_handle);

    /* We don't send vmo handles atm. Vmar has refs to them */

    /* Write to ch1 */
    assert(!ch1->write_msg(NULL, 0, &handle_table[0], 4));

    /* Start test process */
    assert(spawn_zircon_proc(test_thread, stack_vmo, stack_map->get_base(),
            "zircon-test", entry, channel_handle));
    test_proc->thread_started();

    /* Destroy ch0 */
    destroy_object(ch0);
}
