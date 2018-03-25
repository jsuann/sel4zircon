#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

class VmRegion {
public:
    /* Pure virtual */
    virtual void VmRegion() = 0;
    virtual uintptr_t get_start_address() const = 0;
    virtual bool is_vmar() const = 0;
    virtual bool is_vmo_mapping() const = 0;
};

bool cmp(VmRegion *a, VmRegion *b)
{
    return (a->get_start_address() < b->get_start_address());
}
