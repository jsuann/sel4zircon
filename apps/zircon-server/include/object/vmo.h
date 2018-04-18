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

class VmoMapping;

class ZxVmo final : public ZxObject {
public:
    ZxVmo(uint32_t num_pages) : num_pages_{num_pages}, map_list_{this} {
        size_ = num_pages * (1 << seL4_PageBits);
    }

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_VMO; }

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

    VmoMapping *create_mapping(uintptr_t start_addr, ZxVmar *vmar, uint32_t flags);
    void delete_mapping(VmoMapping *vmap);

    bool commit_page(uint32_t index, VmoMapping *vmap);
    bool commit_all_pages(VmoMapping *vmap) {
        for (uint32_t i = 0; i < num_pages_; ++i) {
            if (!commit_page(i, vmap)) {
                return false;
            }
        }
        return true;
    }

    bool commit_range(uint64_t offset, size_t len) {
        uint32_t start_page = offset / (1 << seL4_PageBits);
        uint32_t end_page = (offset + len - 1) / (1 << seL4_PageBits);
        assert(end_page < num_pages_);
        for (uint32_t i = start_page; i <= end_page; ++i) {
            if (!commit_page(i, NULL)) {
                return false;
            }
        }
        return true;
    }

    void decommit_page(uint32_t index);

private:
    /* Server mapping address */
    uintptr_t kaddr_ = 0;

    uint32_t num_pages_;
    size_t size_;

    /* TODO Cache policy */

    /* TODO Clone info */

    /* Frame objects of the vmo */
    vka_object_t *frames_ = NULL;

    /* process mappings of the vmo */
    LinkedList<VmoMapping> map_list_;
};

/* Class representing the mapping of vmo in a vmar */
class VmoMapping final : public Listable<VmoMapping>, public VmRegion {
    friend class ZxVmo;
public:
    VmoMapping(uintptr_t start_addr, seL4_CPtr *caps,
            seL4_CapRights_t rights, ZxVmar *parent) :
        start_addr_{start_addr}, caps_{caps},
        rights_{rights}, parent_{parent} {}

    uintptr_t get_base() const override { return start_addr_; }
    size_t get_size() const override { return ((ZxVmo*)get_owner())->get_size(); }
    VmRegion *get_parent() const override { return parent_; }
    bool is_vmar() const override { return false; }
    bool is_vmo_mapping() const override { return true; }

private:
    /* start address of vmo in process */
    uintptr_t start_addr_;
    /* caps to page frames */
    seL4_CPtr *caps_;
    /* access rights to frames */
    seL4_CapRights_t rights_;
    /* ptr back to owning vmar */
    ZxVmar *parent_;
};
