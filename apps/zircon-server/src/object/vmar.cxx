#include "object/vmar.h"

namespace VmarCxx {

constexpr size_t kVmarMinGap = (1 << seL4_PageBits);
constexpr uintptr_t kAlignMask = ~((1 << seL4_PageBits) - 1);

/* Get an offset for a randomised mapping between start & end */
uintptr_t get_vmar_offset(uintptr_t start, uintptr_t end, size_t size)
{
    uintptr_t rand_addr = rand() & kAlignMask;
    dprintf(SPEW, "Rand addr at %lx, start %lx end %lx\n", rand_addr, start, end);
    uintptr_t offset = rand_addr % (end - size - start);
    return start + offset;
}

uintptr_t get_vmar_offset_compact(uintptr_t start, uintptr_t end, size_t size)
{
    uintptr_t offset = (rand() & kAlignMask) % (end - size - start);
    return start + (offset % (16 * kVmarMinGap));
}

}

void ZxVmar::destroy()
{
    /* VMAR memory should only be freed after it is deactivated. */
    assert(!is_active_);
}

/* Check that a child vm region can fit in vmar */
bool ZxVmar::check_vm_region(uintptr_t child_base, size_t child_size)
{
    uintptr_t child_end = child_base + child_size;

    /* Check for overflow */
    if (child_end < child_base) {
        return false;
    }

    /* Check that child is contained in vmar */
    if (child_base < base_ || child_end > (base_ + size_)) {
        return false;
    }

    /* Check that child doesn't conflict with other mappings */
    VmRegion **vmr = children_.get();
    /* TODO get a lower bound for i. We only need to start checks
       at the greatest vmr_base < child_base */
    for (size_t i = 0; i < children_.size(); ++i) {
        uintptr_t vmr_base = vmr[i]->get_base();
        uintptr_t vmr_end = vmr_base + vmr[i]->get_size();
        if (child_base < vmr_end && child_end > vmr_base) {
            return false;
        } else if (child_base > vmr_end) {
            /* Nothing more to check */
            break;
        }
    }

    /* Ok to add child as subregion */
    return true;
}

/* Add a child vm region. Assumes it has been checked prior */
bool ZxVmar::add_vm_region(VmRegion *child)
{
    assert(child->get_parent() == this);
    return children_.insert(child);
}

zx_status_t ZxVmar::unmap_regions(uintptr_t addr, size_t len)
{
    /* Before attempting any unmappings, we need to verify
       that [addr, addr + len) doesn't partially cover a region */
    VmRegion **vmr = children_.get();
    for (size_t i = 0; i < children_.size(); ++i) {
        uintptr_t vmr_base = vmr[i]->get_base();
        uintptr_t vmr_end = vmr_base + vmr[i]->get_size();
        if (addr > vmr_base && addr < vmr_end) {
            return ZX_ERR_INVALID_ARGS;
        } else if ((addr + len) > vmr_base && (addr + len) < vmr_end) {
            return ZX_ERR_INVALID_ARGS;
        }
    }

    /* Args ok, proceed with unmapping. */
    size_t unmap_count = 0;
    for (size_t i = 0; i < children_.size(); ++i) {
        uintptr_t vmr_base = vmr[i]->get_base();
        uintptr_t vmr_end = vmr_base + vmr[i]->get_size();
        if (vmr_base >= addr && vmr_end <= (addr + len)) {
            if (vmr[i]->is_vmar()) {
                /* Destroy the child vmar */
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
            unmap_count++;
        }
    }

    /* Notify caller if we didn't find anything to unmap */
    return (unmap_count > 0) ? ZX_OK : ZX_ERR_NOT_FOUND;
}

zx_status_t ZxVmar::update_prot(uintptr_t addr, size_t len, uint32_t flags)
{
    /* [addr, addr+len) must only cover whole vmo mappings (no spaces) */
    VmRegion **vmr = children_.get();
    uintptr_t curr_base = addr;
    size_t first_region = 0;
    size_t last_region = 0;
    for (size_t i = 0; i < children_.size(); ++i) {
        uintptr_t vmr_base = vmr[i]->get_base();
        uintptr_t vmr_end = vmr_base + vmr[i]->get_size();
        if (vmr_end < curr_base) {
            ++first_region;
            continue;
        } else if (vmr_base != curr_base) {
            return ZX_ERR_NOT_FOUND;
        } else if (vmr[i]->is_vmar()) {
            return ZX_ERR_INVALID_ARGS;
        }
        /* We have vmo mapping. Check it has the required rights.
           If vmap ends at addr + len, we can finish check */
        VmoMapping *vmap = (VmoMapping *)vmr[i];
        curr_base = vmr_end;
        if (!vmap->check_prot_flags(flags)) {
            return ZX_ERR_ACCESS_DENIED;
        } else if (curr_base == addr + len) {
            last_region = i;
            break;
        }
    }

    if (curr_base != (addr + len)) {
        return ZX_ERR_NOT_FOUND;
    }

    /* Update protections from first_region to last region */
    for (size_t i = first_region; i <= last_region; ++i) {
        VmoMapping *vmap = (VmoMapping *)vmr[i];
        vmap->remap_pages(flags);
    }
    return ZX_OK;
}

uintptr_t ZxVmar::allocate_vm_region_base(uintptr_t size, uint32_t flags)
{
    using namespace VmarCxx;

    /* Check if we can even handle a mapping of this size */
    if (size > size_) {
        return 0;
    }

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
    /* Binary search for the child vm region */
    int p = 0;
    int r = children_.size() - 1;
    int q = r / 2;

    VmRegion **vmr = children_.get();
    while (p <= r) {
        uintptr_t vmr_base = vmr[q]->get_base();
        uintptr_t vmr_end = vmr_base + vmr[q]->get_size();
        if (addr >= vmr_base && addr < vmr_end) {
            /* Address lies in this region.
               Check if vmap or child vmar */
            if (vmr[q]->is_vmo_mapping()) {
                /* Vmap found */
                return (VmoMapping *)vmr[q];
            } else {
                /* Check subregions of child vmar */
                return ((ZxVmar *)vmr[q])->get_vmap_from_addr(addr);
            }
        } else if (addr < vmr_base) {
            /* Search lower half */
            r = q - 1;
            q = (r + p) / 2;
        } else {
            /* Search upper half */
            p = q + 1;
            q = (r + p) / 2;
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
