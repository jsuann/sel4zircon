#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

#include "linkedlist.h"
#include "object.h"
#include "vmar.h"
#include "../zxcpp/pagearray.h"

class VmoMapping;

class ZxVmo final : public ZxObjectWaitable {
public:
    static constexpr size_t vmoPageSize = (1 << seL4_PageBits);

    ZxVmo(uint32_t num_pages) : num_pages_{num_pages}, map_list_{this} {
        size_ = num_pages * vmoPageSize;
    }

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMO; }
    CookieJar *get_cookie_jar() override { return &cookie_jar_; }

    uintptr_t get_base() const {
        return kaddr_;
    }

    size_t get_size() const {
        return size_;
    }

    bool init();
    void destroy() override;

    bool can_destroy() override {
        /* VMOs stay alive while mappings exist */
        return (zero_handles() && map_list_.empty());
    }

    /* read & write assume args are valid! */
    void read(uint64_t offset, size_t len, void *dest) {
        memcpy(dest, (void *)(kaddr_ + offset), len);
    }
    void write(uint64_t offset, size_t len, void *src) {
        memcpy((void *)(kaddr_ + offset), src, len);
    }

    VmoMapping *create_mapping(uintptr_t base_addr, uint64_t offset,
            size_t len, ZxVmar *vmar, uint32_t flags, zx_rights_t map_rights);
    void delete_mapping(VmoMapping *vmap);

    bool commit_page(uint32_t index, VmoMapping *vmap);
    void decommit_page(uint32_t index);

    bool commit_all_pages(VmoMapping *vmap) {
        for (uint32_t i = 0; i < num_pages_; ++i) {
            if (!commit_page(i, vmap)) {
                return false;
            }
        }

        return true;
    }

    bool commit_range(uint64_t offset, size_t len) {
        uint32_t start_page = offset / vmoPageSize;
        /* We round up len to next page */
        uint32_t end_page = (offset + len + vmoPageSize - 1) / vmoPageSize;

        for (uint32_t i = start_page; i < end_page; ++i) {
            if (!commit_page(i, NULL)) {
                return false;
            }
        }

        return true;
    }

    void decommit_range(uint64_t offset, size_t len) {
        uint32_t start_page = offset / vmoPageSize;
        /* We round up len to next page */
        uint32_t end_page = (offset + len + vmoPageSize - 1) / vmoPageSize;

        for (uint32_t i = start_page; i < end_page; ++i) {
            decommit_page(i);
        }
    }

    zx_status_t resize(size_t new_num_pages);

private:
    /* Server mapping address */
    uintptr_t kaddr_ = 0;

    uint32_t num_pages_;
    size_t size_;

    /* TODO Physical/device/contiguous memory info */

    /* TODO Clone info */

    /* Frame objects of the vmo */
    PageArray<vka_object_t> frames_;

    /* process mappings of the vmo */
    LinkedList<VmoMapping> map_list_;

    CookieJar cookie_jar_;
};

/* Class representing the mapping of vmo in a vmar */
class VmoMapping final : public Listable<VmoMapping>, public VmRegion {
    friend class ZxVmo;
public:
    VmoMapping(uintptr_t base_addr, uint32_t start_page, uint32_t num_pages,
            seL4_CapRights_t rights, ZxVmar *parent, zx_rights_t map_rights) :
        base_addr_{base_addr}, start_page_{start_page}, num_pages_{num_pages},
        rights_{rights}, parent_{parent}, map_rights_{map_rights} {}

    uintptr_t get_base() const override { return base_addr_; }
    size_t get_size() const override { return num_pages_ * ZxVmo::vmoPageSize; }
    VmRegion *get_parent() const override { return parent_; }
    bool is_vmar() const override { return false; }
    bool is_vmo_mapping() const override { return true; }

    /* Convert addr to offset in vmo */
    uint64_t addr_to_offset(uintptr_t addr) {
        return ((start_page_ * ZxVmo::vmoPageSize) + (addr - base_addr_));
    }

    bool commit_page_at_addr(uintptr_t addr) {
        /* Work out the index of the page relative to vmo */
        uint32_t index = start_page_ + ((addr - base_addr_) / ZxVmo::vmoPageSize);
        /* Commit the page */
        return ((ZxVmo *)get_owner())->commit_page(index, this);
    }

    bool check_prot_flags(uint32_t flags) {
        if ((flags & ZX_VM_FLAG_PERM_READ) &&
                !(map_rights_ & ZX_RIGHT_READ)) {
            return false;
        } else if ((flags & ZX_VM_FLAG_PERM_WRITE) &&
                !(map_rights_ & ZX_RIGHT_WRITE)) {
            return false;
        } else if ((flags & ZX_VM_FLAG_PERM_EXECUTE) &&
                !(map_rights_ & ZX_RIGHT_EXECUTE)) {
            return false;
        }

        return true;
    }

    void remap_pages(uint32_t flags);

private:
    /* base address of vmo mapping in vmar */
    uintptr_t base_addr_;
    /* Start and num pages of mapping, relative to vmo */
    uint32_t start_page_;
    uint32_t num_pages_;
    /* caps to page frames */
    PageArray<seL4_CPtr> caps_;
    /* access rights to frames */
    seL4_CapRights_t rights_;
    /* ptr back to owning vmar */
    ZxVmar *parent_;
    /* Allowed rights for protection changes */
    zx_rights_t map_rights_;
};
