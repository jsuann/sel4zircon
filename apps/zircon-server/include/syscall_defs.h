#define NUM_SYSCALLS 17

void sys_null(seL4_MessageInfo_t tag, uint32_t handle);
void sys_handle_close(seL4_MessageInfo_t tag, uint32_t handle);
void sys_handle_replace(seL4_MessageInfo_t tag, uint32_t handle);
void sys_handle_duplicate(seL4_MessageInfo_t tag, uint32_t handle);
void sys_channel_create(seL4_MessageInfo_t tag, uint32_t handle);
void sys_channel_read(seL4_MessageInfo_t tag, uint32_t handle);
void sys_channel_write(seL4_MessageInfo_t tag, uint32_t handle);
void sys_channel_call(seL4_MessageInfo_t tag, uint32_t handle);
void sys_test_0(seL4_MessageInfo_t tag, uint32_t handle);
void sys_test_1(seL4_MessageInfo_t tag, uint32_t handle);
void sys_test_2(seL4_MessageInfo_t tag, uint32_t handle);
void sys_test_3(seL4_MessageInfo_t tag, uint32_t handle);
void sys_test_4(seL4_MessageInfo_t tag, uint32_t handle);
void sys_test_5(seL4_MessageInfo_t tag, uint32_t handle);
void sys_test_6(seL4_MessageInfo_t tag, uint32_t handle);
void sys_test_7(seL4_MessageInfo_t tag, uint32_t handle);
void sys_test_8(seL4_MessageInfo_t tag, uint32_t handle);
