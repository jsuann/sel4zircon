#include "object/vmar.h"

void ZxVmar::destroy()
{
    /* VMAR memory should only be freed after it is deactivated. */
    assert(!is_active_);
}

/* Check that a child vm region can fit in vmar */
/* TODO overlap flag */
bool ZxVmar::check_vm_region(uintptr_t child_base, size_t child_size)
{
    uintptr_t child_end = child_base + child_size;

    /* Check that child is contained in vmar */
    if (child_base < base_ || child_end > (base_ + size_)) {
        return false;
    }

    /* Check that child doesn't conflict with other mappings */
    VmRegion **vmr = children_.get();
    for (size_t i = 0; i < children_.size(); ++i) {
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

    if (!children_.insert(child)) {
        return false;
    }

    /* XXX sanity check */
    VmRegion **vmr = children_.get();
    uintptr_t prev_base = 0;
    for (size_t i = 0; i < children_.size(); ++i) {
        uintptr_t vmr_base = vmr[i]->get_base();
        assert(vmr_base > prev_base);
        prev_base = vmr_base;
    }

    return true;
}

/* Lookup a VMO mapping with a vaddr */
VmoMapping *ZxVmar::get_vmap_from_addr(uintptr_t addr)
{
    VmRegion **vmr = children_.get();
    for (size_t i = 0; i < children_.size(); ++i) {
        uintptr_t vmr_base = vmr[i]->get_base();
        uintptr_t vmr_end = vmr_base + vmr[i]->get_size();
        if (addr >= vmr_base && addr < vmr_end) {
            /* Check if vmap or child vmar */
            if (vmr[i]->is_vmo_mapping()) {
                /* Vmap found */
                return (VmoMapping *)vmr[i];
            } else {
                /* Check subregions of child vmar */
                return ((ZxVmar *)vmr[i])->get_vmap_from_addr(addr);
            }
        }
    }

    return NULL;
}

/* Called by vmar_destroy or dead process */
void deactivate_maybe_destroy_vmar(ZxVmar *root)
{
    /* Go through vm regions, and try to destroy them */
    VmRegion **vmr = root->children_.get();
    for (size_t i = 0; i < root->children_.size(); ++i) {
        if (vmr[i]->is_vmar()) {
            /* Recurse on child vmar */
            deactivate_maybe_destroy_vmar((ZxVmar *)vmr[i]);
        } else {
            /* Get VMO and remove mapping */
            VmoMapping *vmap = (VmoMapping *)vmr[i];
            ZxVmo *vmo = (ZxVmo *)vmap->get_owner();
            vmo->delete_mapping(vmap);
            /* If that mapping was last ref to vmo, destroy it */
            if (vmo->can_destroy()) {
                destroy_object(vmo);
            }
        }
    }

    /* Clear vector */
    root->children_.clear();

    /* Set as not active */
    root->is_active_ = false;

    /* If no handle refs, vmar can be freed */
    if (root->can_destroy()) {
        destroy_object(root);
    }
}
