// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/types.h>
#include <zircon/compiler.h>
#include <stdint.h>

__BEGIN_CDECLS

// Accessors for Zircon-specific state maintained by the language runtime

// Examines the set of handles received at process startup for one matching
// |hnd_info|.  If one is found, atomically returns it and removes it from the
// set available to future calls.
// |hnd_info| is a value returned by PA_HND().
zx_handle_t zx_get_startup_handle(uint32_t hnd_info);

zx_handle_t zx_thread_self(void);

zx_handle_t zx_process_self(void);

zx_handle_t zx_vmar_root_self(void);

zx_handle_t zx_job_default(void);

#ifdef CONFIG_HAVE_SEL4ZIRCON

/* Since we are using seL4 libc, we need to manually fetch
   the startup handles from the init channel, which can be
   obtained from argv */
void zx_init_startup_handles(char **argv);

/* Get the root resource handle */
zx_handle_t zx_resource_root(void);

/* We define our own order for startup handles */
#define SZX_PROC_SELF       0x0
#define SZX_THREAD_SELF     0x1
#define SZX_JOB_DEFAULT     0x2
#define SZX_VMAR_ROOT       0x3
#define SZX_RESOURCE_ROOT   0x4

#define SZX_NUM_HANDLES     0x5

#endif

__END_CDECLS
