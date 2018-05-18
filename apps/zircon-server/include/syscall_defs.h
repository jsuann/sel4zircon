/* This is an autogenerated file, do not edit. */
uint64_t sys_clock_get(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_nanosleep(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_ticks_per_second(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_system_get_num_cpus(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_handle_close(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_handle_duplicate(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_handle_replace(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_object_wait_one(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_object_wait_many(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_object_signal(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_object_signal_peer(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_channel_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_channel_read(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_channel_read_etc(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_channel_write(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_socket_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_socket_write(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_socket_read(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_socket_share(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_socket_accept(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_thread_exit(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_thread_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_thread_start(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_process_exit(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_process_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_process_start(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_process_read_memory(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_process_write_memory(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_job_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_task_suspend(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_task_resume(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_task_kill(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_event_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_eventpair_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_timer_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_timer_set(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_timer_cancel(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmo_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmo_read(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmo_write(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmo_get_size(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmo_set_size(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmo_op_range(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmar_allocate(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmar_destroy(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmar_map(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmar_unmap(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_vmar_protect(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_fifo_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_fifo_read(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_fifo_write(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_debug_write(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_syscall_test_0(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_syscall_test_1(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_syscall_test_2(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_syscall_test_3(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_syscall_test_4(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_syscall_test_5(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_syscall_test_6(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_syscall_test_7(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_syscall_test_8(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_syscall_test_wrapper(seL4_MessageInfo_t tag, uint64_t badge);
/* sel4zircon syscalls defined below */
uint64_t sys_debug_putchar(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_endpoint_create(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_endpoint_mint_cap(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_endpoint_delete_cap(seL4_MessageInfo_t tag, uint64_t badge);
uint64_t sys_get_elf_vmo(seL4_MessageInfo_t tag, uint64_t badge);

#define NUM_SYSCALLS 148
