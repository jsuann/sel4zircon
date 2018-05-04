/* This is an autogenerated file, do not edit. */

extern zx_time_t zx_clock_get(uint32_t clock_id);

extern zx_status_t zx_nanosleep(zx_time_t deadline);

extern uint64_t zx_ticks_get(void);

extern uint64_t zx_ticks_per_second(void);

extern zx_time_t zx_deadline_after(zx_duration_t nanoseconds);

extern zx_status_t zx_clock_adjust(zx_handle_t handle, uint32_t clock_id, int64_t offset);

extern uint32_t zx_system_get_num_cpus(void);

extern zx_status_t zx_system_get_version(char* version, uint32_t version_len);

extern uint64_t zx_system_get_physmem(void);

extern zx_status_t zx_system_get_features(uint32_t kind, uint32_t* out);

extern zx_status_t zx_handle_close(zx_handle_t handle);

extern zx_status_t zx_handle_duplicate(zx_handle_t handle, zx_rights_t rights, zx_handle_t* out);

extern zx_status_t zx_handle_replace(zx_handle_t handle, zx_rights_t rights, zx_handle_t* out);

extern zx_status_t zx_object_wait_one(zx_handle_t handle, zx_signals_t waitfor, zx_time_t deadline, zx_signals_t* observed);

extern zx_status_t zx_object_wait_many(zx_wait_item_t* items, uint32_t count, zx_time_t deadline);

extern zx_status_t zx_object_wait_async(zx_handle_t handle, zx_handle_t port_handle, uint64_t key, zx_signals_t signals, uint32_t options);

extern zx_status_t zx_object_signal(zx_handle_t handle, uint32_t clear_mask, uint32_t set_mask);

extern zx_status_t zx_object_signal_peer(zx_handle_t handle, uint32_t clear_mask, uint32_t set_mask);

extern zx_status_t zx_object_get_property(zx_handle_t handle, uint32_t property, void* value, size_t size);

extern zx_status_t zx_object_set_property(zx_handle_t handle, uint32_t property, void* value, size_t size);

extern zx_status_t zx_object_set_cookie(zx_handle_t handle, zx_handle_t scope, uint64_t cookie);

extern zx_status_t zx_object_get_cookie(zx_handle_t handle, zx_handle_t scope, uint64_t* cookie);

extern zx_status_t zx_object_get_info(zx_handle_t handle, uint32_t topic, void* buffer, size_t buffer_size, size_t* actual_count, size_t* avail_count);

extern zx_status_t zx_object_get_child(zx_handle_t handle, uint64_t koid, zx_rights_t rights, zx_handle_t* out);

extern zx_status_t zx_channel_create(uint32_t options, zx_handle_t* out0, zx_handle_t* out1);

extern zx_status_t zx_channel_read(zx_handle_t handle, uint32_t options, void* bytes, zx_handle_t* handles, uint32_t num_bytes, uint32_t num_handles, uint32_t* actual_bytes, uint32_t* actual_handles);

extern zx_status_t zx_channel_read_etc(zx_handle_t handle, uint32_t options, void* bytes, zx_handle_info_t* handles, uint32_t num_bytes, uint32_t num_handles, uint32_t* actual_bytes, uint32_t* actual_handles);

extern zx_status_t zx_channel_write(zx_handle_t handle, uint32_t options, void* bytes, uint32_t num_bytes, zx_handle_t* handles, uint32_t num_handles);

extern zx_status_t zx_channel_call(zx_handle_t handle, uint32_t options, zx_time_t deadline, zx_channel_call_args_t* args, uint32_t* actual_bytes, uint32_t* actual_handles, zx_status_t* read_status);

extern zx_status_t zx_socket_create(uint32_t options, zx_handle_t* out0, zx_handle_t* out1);

extern zx_status_t zx_socket_write(zx_handle_t handle, uint32_t options, void* buffer, size_t size, size_t* actual);

extern zx_status_t zx_socket_read(zx_handle_t handle, uint32_t options, void* buffer, size_t size, size_t* actual);

extern zx_status_t zx_socket_share(zx_handle_t handle, zx_handle_t socket_to_share);

extern zx_status_t zx_socket_accept(zx_handle_t handle, zx_handle_t* out_socket);

extern void zx_thread_exit(void);

extern zx_status_t zx_thread_create(zx_handle_t process, const char* name, uint32_t name_len, uint32_t options, zx_handle_t* out);

extern zx_status_t zx_thread_start(zx_handle_t handle, uintptr_t thread_entry, uintptr_t stack, uintptr_t arg1, uintptr_t arg2);

extern zx_status_t zx_thread_read_state(zx_handle_t handle, uint32_t kind, void* buffer, size_t len);

extern zx_status_t zx_thread_write_state(zx_handle_t handle, uint32_t kind, void* buffer, size_t buffer_len);

extern zx_status_t zx_thread_set_priority(int32_t prio);

extern void zx_process_exit(int retcode);

extern zx_status_t zx_process_create(zx_handle_t job, const char* name, uint32_t name_len, uint32_t options, zx_handle_t* proc_handle, zx_handle_t* vmar_handle);

extern zx_status_t zx_process_start(zx_handle_t process_handle, zx_handle_t thread_handle, uintptr_t entry, uintptr_t stack, zx_handle_t arg_handle, uintptr_t arg2);

extern zx_status_t zx_process_read_memory(zx_handle_t proc, uintptr_t vaddr, void* buffer, size_t len, size_t* actual);

extern zx_status_t zx_process_write_memory(zx_handle_t proc, uintptr_t vaddr, void* buffer, size_t len, size_t* actual);

extern zx_status_t zx_job_create(zx_handle_t parent_job, uint32_t options, zx_handle_t* out);

extern zx_status_t zx_job_set_policy(zx_handle_t job, uint32_t options, uint32_t topic, void* policy, uint32_t count);

extern zx_status_t zx_task_bind_exception_port(zx_handle_t object, zx_handle_t eport, uint64_t key, uint32_t options);

extern zx_status_t zx_task_suspend(zx_handle_t task_handle);

extern zx_status_t zx_task_resume(zx_handle_t task_handle, uint32_t options);

extern zx_status_t zx_task_kill(zx_handle_t task_handle);

extern zx_status_t zx_event_create(uint32_t options, zx_handle_t* out);

extern zx_status_t zx_eventpair_create(uint32_t options, zx_handle_t* out0, zx_handle_t* out1);

extern zx_status_t zx_futex_wait(zx_futex_t* value_ptr, int current_value, zx_time_t deadline);

extern zx_status_t zx_futex_wake(zx_futex_t* value_ptr, uint32_t count);

extern zx_status_t zx_futex_requeue(zx_futex_t* wake_ptr, uint32_t wake_count, int current_value, zx_futex_t* requeue_ptr, uint32_t requeue_count);

extern zx_status_t zx_port_create(uint32_t options, zx_handle_t* out);

extern zx_status_t zx_port_queue(zx_handle_t handle, zx_port_packet_t* packet, size_t count);

extern zx_status_t zx_port_wait(zx_handle_t handle, zx_time_t deadline, zx_port_packet_t* packet, size_t count);

extern zx_status_t zx_port_cancel(zx_handle_t handle, zx_handle_t source, uint64_t key);

extern zx_status_t zx_timer_create(uint32_t options, uint32_t clock_id, zx_handle_t* out);

extern zx_status_t zx_timer_set(zx_handle_t handle, zx_time_t deadline, zx_duration_t slack);

extern zx_status_t zx_timer_cancel(zx_handle_t handle);

extern zx_status_t zx_vmo_create(uint64_t size, uint32_t options, zx_handle_t* out);

extern zx_status_t zx_vmo_read(zx_handle_t handle, void* data, uint64_t offset, size_t len, size_t* actual);

extern zx_status_t zx_vmo_write(zx_handle_t handle, void* data, uint64_t offset, size_t len, size_t* actual);

extern zx_status_t zx_vmo_get_size(zx_handle_t handle, uint64_t* size);

extern zx_status_t zx_vmo_set_size(zx_handle_t handle, uint64_t size);

extern zx_status_t zx_vmo_op_range(zx_handle_t handle, uint32_t op, uint64_t offset, uint64_t size, void* buffer, size_t buffer_size);

extern zx_status_t zx_vmo_clone(zx_handle_t handle, uint32_t options, uint64_t offset, uint64_t size, zx_handle_t* out);

extern zx_status_t zx_vmo_set_cache_policy(zx_handle_t handle, uint32_t cache_policy);

extern zx_status_t zx_vmar_allocate(zx_handle_t parent_vmar_handle, size_t offset, size_t size, uint32_t map_flags, zx_handle_t* child_vmar, uintptr_t* child_addr);

extern zx_status_t zx_vmar_destroy(zx_handle_t vmar_handle);

extern zx_status_t zx_vmar_map(zx_handle_t vmar_handle, size_t vmar_offset, zx_handle_t vmo_handle, uint64_t vmo_offset, size_t len, uint32_t map_flags, uintptr_t* mapped_addr);

extern zx_status_t zx_vmar_unmap(zx_handle_t vmar_handle, uintptr_t addr, size_t len);

extern zx_status_t zx_vmar_protect(zx_handle_t vmar_handle, uintptr_t addr, size_t len, uint32_t prot_flags);

extern zx_status_t zx_cprng_draw(void* buffer, size_t len, size_t* actual);

extern zx_status_t zx_cprng_add_entropy(void* buffer, size_t len);

extern zx_status_t zx_fifo_create(uint32_t elem_count, uint32_t elem_size, uint32_t options, zx_handle_t* out0, zx_handle_t* out1);

extern zx_status_t zx_fifo_read(zx_handle_t handle, void* data, size_t len, uint32_t* num_written);

extern zx_status_t zx_fifo_write(zx_handle_t handle, void* data, size_t len, uint32_t* num_written);

extern zx_status_t zx_vmar_unmap_handle_close_thread_exit(zx_handle_t vmar_handle, uintptr_t addr, size_t len, zx_handle_t handle);

extern void zx_futex_wake_handle_close_thread_exit(zx_futex_t* value_ptr, uint32_t count, int new_value, zx_handle_t handle);

extern zx_status_t zx_log_create(uint32_t options, zx_handle_t* out);

extern zx_status_t zx_log_write(zx_handle_t handle, uint32_t len, void* buffer, uint32_t options);

extern zx_status_t zx_log_read(zx_handle_t handle, uint32_t len, void* buffer, uint32_t options);

extern zx_status_t zx_debuglog_create(zx_handle_t resource, uint32_t options, zx_handle_t* out);

extern zx_status_t zx_debuglog_write(zx_handle_t handle, uint32_t options, void* buffer, size_t len);

extern zx_status_t zx_debuglog_read(zx_handle_t handle, uint32_t options, void* buffer, size_t len);

extern zx_status_t zx_ktrace_read(zx_handle_t handle, void* data, uint32_t offset, uint32_t len, uint32_t* actual);

extern zx_status_t zx_ktrace_control(zx_handle_t handle, uint32_t action, uint32_t options, void* ptr);

extern zx_status_t zx_ktrace_write(zx_handle_t handle, uint32_t id, uint32_t arg0, uint32_t arg1);

extern zx_status_t zx_mtrace_control(zx_handle_t handle, uint32_t kind, uint32_t action, uint32_t options, void* ptr, uint32_t size);

extern zx_status_t zx_debug_read(zx_handle_t handle, void* buffer, uint32_t length);

extern zx_status_t zx_debug_write(void* buffer, uint32_t length);

extern zx_status_t zx_debug_send_command(zx_handle_t resource_handle, void* buffer, uint32_t length);

extern zx_status_t zx_interrupt_create(zx_handle_t hrsrc, uint32_t options, zx_handle_t* out_handle);

extern zx_status_t zx_interrupt_bind(zx_handle_t handle, uint32_t slot, zx_handle_t hrsrc, uint32_t vector, uint32_t options);

extern zx_status_t zx_interrupt_wait(zx_handle_t handle, uint64_t* slots);

extern zx_status_t zx_interrupt_get_timestamp(zx_handle_t handle, uint32_t slot, zx_time_t* timestamp);

extern zx_status_t zx_interrupt_signal(zx_handle_t handle, uint32_t slot, zx_time_t timestamp);

extern zx_status_t zx_mmap_device_io(zx_handle_t handle, uint32_t io_addr, uint32_t len);

extern zx_status_t zx_vmo_create_contiguous(zx_handle_t rsrc_handle, size_t size, uint32_t alignment_log2, zx_handle_t* out);

extern zx_status_t zx_vmo_create_physical(zx_handle_t rsrc_handle, zx_paddr_t paddr, size_t size, zx_handle_t* out);

extern zx_status_t zx_iommu_create(zx_handle_t rsrc_handle, uint32_t type, void* desc, uint32_t desc_len, zx_handle_t* out);

extern zx_status_t zx_bti_create(zx_handle_t iommu, uint32_t options, uint64_t bti_id, zx_handle_t* out);

extern zx_status_t zx_bti_pin(zx_handle_t bti, zx_handle_t vmo, uint64_t offset, uint64_t size, uint32_t perms, zx_paddr_t* addrs, size_t addrs_len, size_t* actual);

extern zx_status_t zx_bti_unpin(zx_handle_t bti, zx_paddr_t base_addr);

extern zx_status_t zx_bootloader_fb_get_info(uint32_t* format, uint32_t* width, uint32_t* height, uint32_t* stride);

extern zx_status_t zx_set_framebuffer(zx_handle_t handle, void* vaddr, uint32_t len, uint32_t format, uint32_t width, uint32_t height, uint32_t stride);

extern zx_status_t zx_set_framebuffer_vmo(zx_handle_t handle, zx_handle_t vmo, uint32_t len, uint32_t format, uint32_t width, uint32_t height, uint32_t stride);

extern zx_status_t zx_pci_get_nth_device(zx_handle_t handle, uint32_t index, zx_pcie_device_info_t* out_info, zx_handle_t* out_handle);

extern zx_status_t zx_pci_enable_bus_master(zx_handle_t handle, bool enable);

extern zx_status_t zx_pci_reset_device(zx_handle_t handle);

extern zx_status_t zx_pci_config_read(zx_handle_t handle, uint16_t offset, size_t width, uint32_t* out_val);

extern zx_status_t zx_pci_config_write(zx_handle_t handle, uint16_t offset, size_t width, uint32_t val);

extern zx_status_t zx_pci_cfg_pio_rw(zx_handle_t handle, uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t* val, size_t width, bool write);

extern zx_status_t zx_pci_get_bar(zx_handle_t handle, uint32_t bar_num, zx_pci_bar_t* out_bar, zx_handle_t* out_handle);

extern zx_status_t zx_pci_map_interrupt(zx_handle_t handle, int32_t which_irq, zx_handle_t* out_handle);

extern zx_status_t zx_pci_query_irq_mode(zx_handle_t handle, uint32_t mode, uint32_t* out_max_irqs);

extern zx_status_t zx_pci_set_irq_mode(zx_handle_t handle, uint32_t mode, uint32_t requested_irq_count);

extern zx_status_t zx_pci_init(zx_handle_t handle, zx_pci_init_arg_t* init_buf, uint32_t len);

extern zx_status_t zx_pci_add_subtract_io_range(zx_handle_t handle, bool mmio, uint64_t base, uint64_t len, bool add);

extern uint64_t zx_acpi_uefi_rsdp(zx_handle_t handle);

extern zx_status_t zx_resource_create(zx_handle_t parent_handle, uint32_t kind, uint64_t low, uint64_t high, zx_handle_t* resource_out);

extern zx_status_t zx_guest_create(zx_handle_t resource, uint32_t options, zx_handle_t physmem_vmo, zx_handle_t* out);

extern zx_status_t zx_guest_set_trap(zx_handle_t guest, uint32_t kind, zx_vaddr_t addr, size_t len, zx_handle_t port, uint64_t key);

extern zx_status_t zx_vcpu_create(zx_handle_t guest, uint32_t options, zx_vaddr_t entry, zx_handle_t* out);

extern zx_status_t zx_vcpu_resume(zx_handle_t vcpu, zx_port_packet_t* packet);

extern zx_status_t zx_vcpu_interrupt(zx_handle_t vcpu, uint32_t vector);

extern zx_status_t zx_vcpu_read_state(zx_handle_t vcpu, uint32_t kind, void* buffer, uint32_t len);

extern zx_status_t zx_vcpu_write_state(zx_handle_t vcpu, uint32_t kind, void* buffer, uint32_t len);

extern zx_status_t zx_system_mexec(zx_handle_t kernel, zx_handle_t bootimage);

extern zx_status_t zx_system_powerctl(zx_handle_t root_rsrc, uint32_t cmd, zx_system_powerctl_arg_t* arg);

extern zx_status_t zx_job_set_relative_importance(zx_handle_t root_resource, zx_handle_t job, zx_handle_t less_important_job);

extern zx_status_t zx_syscall_test_0(void);

extern zx_status_t zx_syscall_test_1(int a);

extern zx_status_t zx_syscall_test_2(int a, int b);

extern zx_status_t zx_syscall_test_3(int a, int b, int c);

extern zx_status_t zx_syscall_test_4(int a, int b, int c, int d);

extern zx_status_t zx_syscall_test_5(int a, int b, int c, int d, int e);

extern zx_status_t zx_syscall_test_6(int a, int b, int c, int d, int e, int f);

extern zx_status_t zx_syscall_test_7(int a, int b, int c, int d, int e, int f, int g);

extern zx_status_t zx_syscall_test_8(int a, int b, int c, int d, int e, int f, int g, int h);

extern zx_status_t zx_syscall_test_wrapper(int a, int b, int c);
