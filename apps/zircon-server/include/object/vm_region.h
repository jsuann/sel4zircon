#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>
#include "../zxcpp/vector.h"

class VmRegion {
public:
    /* Pure virtual */
    virtual uintptr_t get_base() const = 0;
    virtual size_t get_size() const = 0;
    virtual VmRegion *get_parent() const = 0;
    virtual bool is_vmar() const = 0;
    virtual bool is_vmo_mapping() const = 0;
};

template <>
bool cmp<VmRegion*>(VmRegion *a, VmRegion *b)
{
    return (a->get_base() < b->get_base());
}
