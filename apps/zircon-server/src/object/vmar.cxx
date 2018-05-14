#include "object/vmar.h"

namespace VmarCxx {

constexpr size_t kVmarMinGap = (1 << 16);

/* Get an offset for a randomised mapping between start & end */
uintptr_t get_vmar_offset(uintptr_t start, uintptr_t end, size_t size)
{
    uintptr_t offset = (rand() << 16) % (end - size - start);
    return start + offset;
}

uintptr_t get_vmar_offset_compact(uintptr_t start, uintptr_t end, size_t size)
{
    uintptr_t offset = (rand() << 16) % (end - size - start);
    return start + (offset % (16 * kVmarMinGap));
}

}

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

uintptr_t ZxVmar::allocate_vm_region_base(uintptr_t size, uint32_t flags)
{
    using namespace VmarCxx;

    /* There are num children + 1 gaps in the vmar
       space. Use rand to pick a gap to try */
    uint32_t num_gaps = children_.size() + 1;
    uint32_t selected_gap = rand() % num_gaps;

    /* From this gap, find the first that can fit the size of the region. */
    VmRegion **vmr = children_.get();
    bool gap_found = false;
    uintptr_t gap_base, gap_end;
    for (size_t i = 0; i < num_gaps; ++i) {
        uint32_t gap_to_try = (i + selected_gap) % num_gaps;
        if (num_gaps == 1) {
            gap_base = base_;
            gap_end = base_ + size_;
        } else if (gap_to_try == 0) {
            gap_base = base_;
            gap_end = vmr[gap_to_try]->get_base();
        } else if (gap_to_try == (num_gaps - 1)) {
            gap_base = vmr_get_end(vmr[gap_to_try - 1]);
            gap_end = base_ + size_;
        } else {
            gap_base = vmr_get_end(vmr[gap_to_try - 1]);
            gap_end = vmr[gap_to_try]->get_base();
        }
        /* Should have some space between regions */
        gap_base += kVmarMinGap;
        gap_end -= kVmarMinGap;
        /* See if region fits */
        if (gap_base + size <= gap_end) {
            selected_gap = gap_to_try;
            gap_found = true;
            break;
        }
    }

    if (!gap_found) {
        return 0;
    }

    /* We have a gap. Next we randomly select an offset in the gap */
    if (num_gaps > 1 && (flags & ZX_VM_FLAG_COMPACT)) {
        /* Try to make mapping close to other mappings */
        uintptr_t offset = get_vmar_offset_compact(gap_base, gap_end, size);
        /* If we have 0th gap, align to 0th vmr */
        if (selected_gap == 0) {
            offset = gap_end - size - (offset - gap_base);
        }
        return offset;
    } else {
        /* Just select anywhere in the region */
        return get_vmar_offset(gap_base, gap_end, size);
    }
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
