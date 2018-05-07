#include "object/resource.h"
#include "object/process.h"

namespace ResourceCxx {
    ZxResource *root_resource;
};

void init_root_resource()
{
    using namespace ResourceCxx;
    root_resource = allocate_object<ZxResource>(ZX_RSRC_KIND_ROOT, 0, 0);
    assert(root_resource != NULL);
}

ZxResource *get_root_resource()
{
    return ResourceCxx::root_resource;
}

zx_status_t validate_resource(ZxProcess *proc, zx_handle_t handle, uint32_t kind)
{
    ZxResource *resource;
    zx_status_t status = proc->get_object(handle, resource);
    if (status != ZX_OK) {
        return status;
    }

    uint32_t rkind = resource->get_kind();
    if ((rkind == ZX_RSRC_KIND_ROOT) || (rkind == kind)) {
        return ZX_OK;
    }
    return ZX_ERR_ACCESS_DENIED;
}

zx_status_t validate_ranged_resource(ZxProcess *proc, zx_handle_t handle,
        uint32_t kind, uint64_t low, uint64_t high)
{
    ZxResource *resource;
    zx_status_t status = proc->get_object(handle, resource);
    if (status != ZX_OK) {
        return status;
    }

    uint32_t rsrc_kind = resource->get_kind();
    if (rsrc_kind == ZX_RSRC_KIND_ROOT) {
        return ZX_OK;
    } else if (rsrc_kind == kind) {
        uint64_t rsrc_low, rsrc_high;
        resource->get_range(&rsrc_low, &rsrc_high);
        if (low >= rsrc_low && high <= rsrc_high) {
            return ZX_OK;
        }
    }

    return ZX_ERR_ACCESS_DENIED;
}
