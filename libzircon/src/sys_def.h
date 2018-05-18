/* This is an autogenerated file, do not edit. */
#define ZX_SYS_CLOCK_GET 0
#define ZX_SYS_NANOSLEEP 1
#define ZX_SYS_TICKS_PER_SECOND 2
#define ZX_SYS_CLOCK_ADJUST 3
#define ZX_SYS_SYSTEM_GET_NUM_CPUS 4
#define ZX_SYS_SYSTEM_GET_VERSION 5
#define ZX_SYS_SYSTEM_GET_PHYSMEM 6
#define ZX_SYS_SYSTEM_GET_FEATURES 7
#define ZX_SYS_HANDLE_CLOSE 8
#define ZX_SYS_HANDLE_DUPLICATE 9
#define ZX_SYS_HANDLE_REPLACE 10
#define ZX_SYS_OBJECT_WAIT_ONE 11
#define ZX_SYS_OBJECT_WAIT_MANY 12
#define ZX_SYS_OBJECT_WAIT_ASYNC 13
#define ZX_SYS_OBJECT_SIGNAL 14
#define ZX_SYS_OBJECT_SIGNAL_PEER 15
#define ZX_SYS_OBJECT_GET_PROPERTY 16
#define ZX_SYS_OBJECT_SET_PROPERTY 17
#define ZX_SYS_OBJECT_SET_COOKIE 18
#define ZX_SYS_OBJECT_GET_COOKIE 19
#define ZX_SYS_OBJECT_GET_INFO 20
#define ZX_SYS_OBJECT_GET_CHILD 21
#define ZX_SYS_CHANNEL_CREATE 22
#define ZX_SYS_CHANNEL_READ 23
#define ZX_SYS_CHANNEL_READ_ETC 24
#define ZX_SYS_CHANNEL_WRITE 25
#define ZX_SYS_CHANNEL_CALL 26
#define ZX_SYS_SOCKET_CREATE 27
#define ZX_SYS_SOCKET_WRITE 28
#define ZX_SYS_SOCKET_READ 29
#define ZX_SYS_SOCKET_SHARE 30
#define ZX_SYS_SOCKET_ACCEPT 31
#define ZX_SYS_THREAD_EXIT 32
#define ZX_SYS_THREAD_CREATE 33
#define ZX_SYS_THREAD_START 34
#define ZX_SYS_THREAD_READ_STATE 35
#define ZX_SYS_THREAD_WRITE_STATE 36
#define ZX_SYS_THREAD_SET_PRIORITY 37
#define ZX_SYS_PROCESS_EXIT 38
#define ZX_SYS_PROCESS_CREATE 39
#define ZX_SYS_PROCESS_START 40
#define ZX_SYS_PROCESS_READ_MEMORY 41
#define ZX_SYS_PROCESS_WRITE_MEMORY 42
#define ZX_SYS_JOB_CREATE 43
#define ZX_SYS_JOB_SET_POLICY 44
#define ZX_SYS_TASK_BIND_EXCEPTION_PORT 45
#define ZX_SYS_TASK_SUSPEND 46
#define ZX_SYS_TASK_RESUME 47
#define ZX_SYS_TASK_KILL 48
#define ZX_SYS_EVENT_CREATE 49
#define ZX_SYS_EVENTPAIR_CREATE 50
#define ZX_SYS_FUTEX_WAIT 51
#define ZX_SYS_FUTEX_WAKE 52
#define ZX_SYS_FUTEX_REQUEUE 53
#define ZX_SYS_PORT_CREATE 54
#define ZX_SYS_PORT_QUEUE 55
#define ZX_SYS_PORT_WAIT 56
#define ZX_SYS_PORT_CANCEL 57
#define ZX_SYS_TIMER_CREATE 58
#define ZX_SYS_TIMER_SET 59
#define ZX_SYS_TIMER_CANCEL 60
#define ZX_SYS_VMO_CREATE 61
#define ZX_SYS_VMO_READ 62
#define ZX_SYS_VMO_WRITE 63
#define ZX_SYS_VMO_GET_SIZE 64
#define ZX_SYS_VMO_SET_SIZE 65
#define ZX_SYS_VMO_OP_RANGE 66
#define ZX_SYS_VMO_CLONE 67
#define ZX_SYS_VMO_SET_CACHE_POLICY 68
#define ZX_SYS_VMAR_ALLOCATE 69
#define ZX_SYS_VMAR_DESTROY 70
#define ZX_SYS_VMAR_MAP 71
#define ZX_SYS_VMAR_UNMAP 72
#define ZX_SYS_VMAR_PROTECT 73
#define ZX_SYS_CPRNG_DRAW 74
#define ZX_SYS_CPRNG_ADD_ENTROPY 75
#define ZX_SYS_FIFO_CREATE 76
#define ZX_SYS_FIFO_READ 77
#define ZX_SYS_FIFO_WRITE 78
#define ZX_SYS_VMAR_UNMAP_HANDLE_CLOSE_THREAD_EXIT 79
#define ZX_SYS_FUTEX_WAKE_HANDLE_CLOSE_THREAD_EXIT 80
#define ZX_SYS_LOG_CREATE 81
#define ZX_SYS_LOG_WRITE 82
#define ZX_SYS_LOG_READ 83
#define ZX_SYS_DEBUGLOG_CREATE 84
#define ZX_SYS_DEBUGLOG_WRITE 85
#define ZX_SYS_DEBUGLOG_READ 86
#define ZX_SYS_KTRACE_READ 87
#define ZX_SYS_KTRACE_CONTROL 88
#define ZX_SYS_KTRACE_WRITE 89
#define ZX_SYS_MTRACE_CONTROL 90
#define ZX_SYS_DEBUG_READ 91
#define ZX_SYS_DEBUG_WRITE 92
#define ZX_SYS_DEBUG_SEND_COMMAND 93
#define ZX_SYS_INTERRUPT_CREATE 94
#define ZX_SYS_INTERRUPT_BIND 95
#define ZX_SYS_INTERRUPT_WAIT 96
#define ZX_SYS_INTERRUPT_GET_TIMESTAMP 97
#define ZX_SYS_INTERRUPT_SIGNAL 98
#define ZX_SYS_MMAP_DEVICE_IO 99
#define ZX_SYS_VMO_CREATE_CONTIGUOUS 100
#define ZX_SYS_VMO_CREATE_PHYSICAL 101
#define ZX_SYS_IOMMU_CREATE 102
#define ZX_SYS_BTI_CREATE 103
#define ZX_SYS_BTI_PIN 104
#define ZX_SYS_BTI_UNPIN 105
#define ZX_SYS_BOOTLOADER_FB_GET_INFO 106
#define ZX_SYS_SET_FRAMEBUFFER 107
#define ZX_SYS_SET_FRAMEBUFFER_VMO 108
#define ZX_SYS_PCI_GET_NTH_DEVICE 109
#define ZX_SYS_PCI_ENABLE_BUS_MASTER 110
#define ZX_SYS_PCI_RESET_DEVICE 111
#define ZX_SYS_PCI_CONFIG_READ 112
#define ZX_SYS_PCI_CONFIG_WRITE 113
#define ZX_SYS_PCI_CFG_PIO_RW 114
#define ZX_SYS_PCI_GET_BAR 115
#define ZX_SYS_PCI_MAP_INTERRUPT 116
#define ZX_SYS_PCI_QUERY_IRQ_MODE 117
#define ZX_SYS_PCI_SET_IRQ_MODE 118
#define ZX_SYS_PCI_INIT 119
#define ZX_SYS_PCI_ADD_SUBTRACT_IO_RANGE 120
#define ZX_SYS_ACPI_UEFI_RSDP 121
#define ZX_SYS_RESOURCE_CREATE 122
#define ZX_SYS_GUEST_CREATE 123
#define ZX_SYS_GUEST_SET_TRAP 124
#define ZX_SYS_VCPU_CREATE 125
#define ZX_SYS_VCPU_RESUME 126
#define ZX_SYS_VCPU_INTERRUPT 127
#define ZX_SYS_VCPU_READ_STATE 128
#define ZX_SYS_VCPU_WRITE_STATE 129
#define ZX_SYS_SYSTEM_MEXEC 130
#define ZX_SYS_SYSTEM_POWERCTL 131
#define ZX_SYS_JOB_SET_RELATIVE_IMPORTANCE 132
#define ZX_SYS_SYSCALL_TEST_0 133
#define ZX_SYS_SYSCALL_TEST_1 134
#define ZX_SYS_SYSCALL_TEST_2 135
#define ZX_SYS_SYSCALL_TEST_3 136
#define ZX_SYS_SYSCALL_TEST_4 137
#define ZX_SYS_SYSCALL_TEST_5 138
#define ZX_SYS_SYSCALL_TEST_6 139
#define ZX_SYS_SYSCALL_TEST_7 140
#define ZX_SYS_SYSCALL_TEST_8 141
#define ZX_SYS_SYSCALL_TEST_WRAPPER 142
/* sel4zircon syscalls defined below */
#define ZX_SYS_DEBUG_PUTCHAR 143
#define ZX_SYS_ENDPOINT_CREATE 144
#define ZX_SYS_ENDPOINT_MINT_CAP 145
#define ZX_SYS_ENDPOINT_DELETE_CAP 146
#define ZX_SYS_GET_ELF_VMO 147
