#define NUM_SYSCALLS 8

void sys_null(seL4_MessageInfo_t tag, uint32_t handle);
void sys_handle_close(seL4_MessageInfo_t tag, uint32_t handle);
void sys_handle_replace(seL4_MessageInfo_t tag, uint32_t handle);
void sys_handle_duplicate(seL4_MessageInfo_t tag, uint32_t handle);
void sys_channel_create(seL4_MessageInfo_t tag, uint32_t handle);
void sys_channel_read(seL4_MessageInfo_t tag, uint32_t handle);
void sys_channel_write(seL4_MessageInfo_t tag, uint32_t handle);
void sys_channel_call(seL4_MessageInfo_t tag, uint32_t handle);
