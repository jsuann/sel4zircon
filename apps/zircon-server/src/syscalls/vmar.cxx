#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include "debug.h"
}

#include "object/vmar.h"
#include "sys_helpers.h"

namespace SysVmar {

/* Mask to check for alignment */
constexpr size_t align_mask = (1 << seL4_PageBits) - 1;

/* Allowed flags for vmar allocate */
constexpr uint32_t AllocateFlags =
        ZX_VM_FLAG_CAN_MAP_SPECIFIC | ZX_VM_FLAG_CAN_MAP_READ |
        ZX_VM_FLAG_CAN_MAP_WRITE | ZX_VM_FLAG_CAN_MAP_EXECUTE |
        ZX_VM_FLAG_COMPACT | ZX_VM_FLAG_SPECIFIC;

constexpr uint32_t VmoMapFlags =
        ZX_VM_FLAG_SPECIFIC | ZX_VM_FLAG_SPECIFIC_OVERWRITE |
        ZX_VM_FLAG_PERM_READ | ZX_VM_FLAG_PERM_WRITE |
        ZX_VM_FLAG_PERM_EXECUTE | ZX_VM_FLAG_MAP_RANGE;
}

uint64_t sys_vmar_allocate(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 6);
    zx_handle_t parent_vmar_handle = seL4_GetMR(0);
    size_t offset = seL4_GetMR(1);
    size_t size = seL4_GetMR(2);
    uint32_t map_flags = seL4_GetMR(3);
    uintptr_t user_child_vmar = seL4_GetMR(4);
    uintptr_t user_child_addr = seL4_GetMR(5);

    /* Check for valid map flags */
    if (map_flags & ~SysVmar::AllocateFlags) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Offset must be zero if we are doing a random mapping */
    if (!(map_flags & ZX_VM_FLAG_SPECIFIC) && offset != 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Check offset & size page aligned */
    if ((offset & SysVmar::align_mask) || (size & SysVmar::align_mask)) {
        return ZX_ERR_INVALID_ARGS;
    }

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    zx_handle_t *child_vmar;
    uintptr_t *child_addr;
    err = proc->get_kvaddr(user_child_vmar, child_vmar);
    SYS_RET_IF_ERR(err);
    err = proc->get_kvaddr(user_child_addr, child_addr);
    SYS_RET_IF_ERR(err);

    /* Convert desired mapping rights to handle rights */
    zx_rights_t vmar_rights = 0u;
    if (map_flags & ZX_VM_FLAG_CAN_MAP_READ) {
        vmar_rights |= ZX_RIGHT_READ;
    }
    if (map_flags & ZX_VM_FLAG_CAN_MAP_WRITE) {
        vmar_rights |= ZX_RIGHT_WRITE;
    }
    if (map_flags & ZX_VM_FLAG_CAN_MAP_EXECUTE) {
        vmar_rights |= ZX_RIGHT_EXECUTE;
    }

    /* Handle to parent vmar must have these rights */
    ZxVmar *parent;
    err = proc->get_object_with_rights(parent_vmar_handle, vmar_rights, parent);
    SYS_RET_IF_ERR(err);

    /* If vmar has been deactivated, we can't allocate anymore */
    if (parent->is_deactivated()) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* If not doing a specific mapping, allocate an offset */
    if (!(map_flags & ZX_VM_FLAG_SPECIFIC)) {
        offset = parent->allocate_vm_region_base(size, map_flags);
        if (offset == 0) {
            return ZX_ERR_INVALID_ARGS;
        }
    }

    /* Check that offset & size does not overlap with other regions.
       We do this even with allocated offset as a sanity check. */
    if (!parent->check_vm_region(offset, size)) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Now allocate the vmar */
    ZxVmar *new_vmar = allocate_object<ZxVmar>(parent, offset, size, map_flags);
    if (new_vmar == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Create a handle to new vmar */
    Handle *h = new_vmar->create_handle(new_vmar->get_rights());
    if (h == NULL) {
        free_object(new_vmar);
        return ZX_ERR_NO_MEMORY;
    }

    /* Add to parent vmar */
    if (!parent->add_vm_region(new_vmar)) {
        new_vmar->destroy_handle(h);
        free_object(new_vmar);
        return ZX_ERR_NO_MEMORY;
    }

    proc->add_handle(h);
    *child_vmar = proc->get_handle_user_val(h);
    *child_addr = offset;
    return ZX_OK;
}

uint64_t sys_vmar_map(seL4_MessageInfo_t tag, uint64_t badge)
{
    SYS_CHECK_NUM_ARGS(tag, 7);
    zx_handle_t vmar_handle = seL4_GetMR(0);
    size_t vmar_offset = seL4_GetMR(1);
    zx_handle_t vmo_handle = seL4_GetMR(2);
    uint64_t vmo_offset = seL4_GetMR(3);
    size_t len = seL4_GetMR(4);
    uint32_t map_flags = seL4_GetMR(5);
    uintptr_t  user_mapped_addr = seL4_GetMR(6);

    /* Check for valid map flags */
    if (map_flags & ~SysVmar::VmoMapFlags) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* We currently don't handle specific overwrite */
    if (map_flags & ZX_VM_FLAG_SPECIFIC_OVERWRITE) {
        return ZX_ERR_NOT_SUPPORTED;
    }

    /* Vmar offset must be zero if we are doing a random mapping */
    if (!(map_flags & ZX_VM_FLAG_SPECIFIC) && vmar_offset != 0) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Check that vmar & vmo offsets are page aligned */
    if ((vmar_offset & SysVmar::align_mask) || (vmo_offset & SysVmar::align_mask)) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Round up len to page boundary */
    if (len == 0) {
        return ZX_ERR_INVALID_ARGS;
    }
    len = (len + SysVmar::align_mask) & ~SysVmar::align_mask;

    zx_status_t err;
    ZxProcess *proc = get_proc_from_badge(badge);

    uintptr_t *mapped_addr;
    err = proc->get_kvaddr(user_mapped_addr, mapped_addr);
    SYS_RET_IF_ERR(err);

    /* Get rights required by both vmar & vmo */
    zx_rights_t req_rights = 0u;
    if (map_flags & ZX_VM_FLAG_PERM_READ) {
        req_rights |= ZX_RIGHT_READ;
    }
    if (map_flags & ZX_VM_FLAG_PERM_READ) {
        req_rights |= ZX_RIGHT_WRITE;
    }
    if (map_flags & ZX_VM_FLAG_PERM_READ) {
        req_rights |= ZX_RIGHT_EXECUTE;
    }

    /* Get vmar */
    ZxVmar *vmar;
    err = proc->get_object_with_rights(vmar_handle, req_rights, vmar);
    SYS_RET_IF_ERR(err);

    /* Get vmo, which also requires map right */
    ZxVmo *vmo;
    err = proc->get_object_with_rights(vmo_handle, req_rights | ZX_RIGHT_MAP, vmo);
    SYS_RET_IF_ERR(err);

    /* Get the rights of the vmo handle for later */
    zx_rights_t map_rights = proc->get_handle(vmo_handle)->get_rights();

    /* If vmar has been deactivated, we can't map anymore */
    if (vmar->is_deactivated()) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* If not doing a specific mapping, allocate an offset */
    if (!(map_flags & ZX_VM_FLAG_SPECIFIC)) {
        vmar_offset = vmar->allocate_vm_region_base(len, map_flags);
        if (vmar_offset == 0) {
            return ZX_ERR_INVALID_ARGS;
        }
    }

    /* Check that offset & size does not overlap with other regions.
       We do this even with allocated offset as a sanity check. */
    if (!vmar->check_vm_region(vmar_offset, len)) {
        return ZX_ERR_INVALID_ARGS;
    }

    /* Create the vmo mapping. This will also add the mapping to the vmar */
    VmoMapping *vmap = vmo->create_mapping(vmar_offset, vmo_offset, len,
            vmar, map_flags, map_rights);
    if (vmap == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Mapping successful! */
    *mapped_addr = vmar_offset;
    return ZX_OK;
}
