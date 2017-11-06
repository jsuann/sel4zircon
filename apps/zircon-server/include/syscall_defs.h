#define NUM_SYSCALLS 8

void sys_null(uint32_t);
void sys_handle_close(uint32_t);
void sys_handle_replace(uint32_t);
void sys_handle_duplicate(uint32_t);
void sys_channel_create(uint32_t);
void sys_channel_read(uint32_t);
void sys_channel_write(uint32_t);
void sys_channel_call(uint32_t);
