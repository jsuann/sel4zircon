#include "object/vmar.h"

/* Check that a child vm region can fit in vmar */
bool ZxVmar::check_vm_region(uintptr_t child_base, size_t child_size)
{
    uintptr_t child_end = child_base + child_size;

    /* Check that child is contained in vmar */
    if (child_base < base_ || child_end > (base_ + size_)) {
        return false;
    }

    /* Check that child doesn't conflict with other mappings */
    VmRegion **vmr = children_->get();
    for (size_t i = 0; i < children_->size(); ++i) {
        uintptr_t vmr_base = vmr[i]->get_base();
        uintptr_t vmr_end = vmr_base + vmr[i]->get_size();
        if (child_base < vmr_end && child_end > vmr_base) {
            return false;
        }
    }

    /* Ok to add child as subregion */
    return true;
}

/* Add a child vm region */
bool ZxVmar::add_vm_region(VmRegion *child)
{
    assert(child->get_parent() == this);

    if (!children_->insert(child)) {
        return false;
    }

    /* XXX sanity check */
    VmRegion **vmr = children_->get();
    uintptr_t prev_base = 0;
    for (size_t i = 0; i < children_->size(); ++i) {
        uintptr_t vmr_base = vmr[i]->get_base();
        assert(vmr_base > prev_base);
        prev_base = vmr_base;
    }

    return true;
}
