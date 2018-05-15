#include "addrspace.h"
#include "object/vmo.h"
#include "object/process.h"

bool ZxVmo::init()
{
    /* Get a kmap & set kaddr */
    kaddr_ = alloc_vmo_kmap();
    if (kaddr_ == 0) {
        return false;
    }

    /* Init page array of uninitialised frame objects */
    if (!frames_.init(num_pages_)) {
        return false;
    }

    return true;
}

void ZxVmo::destroy()
{
    vka_t *vka = get_server_vka();

    /* Only destroy when no mappings exist */
    assert(map_list_.empty());

    /* Free any allocated frame objects */
    auto free_func = [vka] (vka_object_t &frame) {
        if (frame.cptr != 0) {
            vka_free_object(vka, &frame);
        }
    };
    frames_.clear(free_func);

    /* Free kmap */
    if (kaddr_ != 0) {
        free_vmo_kmap(kaddr_);
    }
}

VmoMapping *ZxVmo::create_mapping(uintptr_t base_addr, uint64_t offset,
        size_t len, ZxVmar *vmar, uint32_t flags, zx_rights_t map_rights)
{
    /* Check overflow */
    if (offset + len < offset) {
        return NULL;
    }

    /* Check offset & len for validity */
    if (offset + len > size_) {
        return NULL;
    }

    /* Alloc mem for vmap */
    void *vmap_mem = malloc(sizeof(VmoMapping));
    if (vmap_mem == NULL) {
        return NULL;
    }

    /* get frame access rights from mapping flags (should be valid by this point!) */
    seL4_CapRights_t rights;
    bool can_read = (flags & ZX_VM_FLAG_PERM_READ || flags & ZX_VM_FLAG_PERM_EXECUTE);
    bool can_write = (flags & ZX_VM_FLAG_PERM_WRITE);
    rights = seL4_CapRights_new(0, can_read, can_write);

    /* Convert offset & len to start page & num pages */
    uint32_t vmap_start_page = offset / vmoPageSize;
    uint32_t vmap_num_pages = len / vmoPageSize;

    /* Create mapping */
    VmoMapping *vmap = new (vmap_mem) VmoMapping(base_addr, vmap_start_page,
            vmap_num_pages, rights, vmar, map_rights);

    /* Init page array for caps. We still use the total num pages
       of the vmo so we don't have to calculate a relative index
       when committing/decommitting pages from mappings. */
    if (!vmap->caps_.init(num_pages_)) {
        delete vmap;
        return NULL;
    }

    map_list_.push_back(vmap);

    /* Add vmap to vmar. We assume it has been checked for validity */
    if (!vmar->add_vm_region(vmap)) {
        delete_mapping(vmap);
        return NULL;
    }

    /* If range flag set, try to map pages committed to vmo */
    if (flags & ZX_VM_FLAG_MAP_RANGE) {
        uint32_t vmap_end_page = vmap_start_page + vmap_num_pages;
        for (uint32_t i = vmap_start_page; i < vmap_end_page; ++i) {
            if (frames_.has(i) && frames_[i].cptr != 0) {
                if (!commit_page(i, vmap)) {
                    delete_mapping(vmap);
                    return NULL;
                }
            }
        }
    }

    return vmap;
}

void ZxVmo::delete_mapping(VmoMapping *vmap)
{
    vka_t *vka = get_server_vka();
    assert(map_list_.contains(vmap));

    /* Clear all caps */
    auto free_func = [vka] (seL4_CPtr &cap) {
        if (cap != 0) {
            cspacepath_t path;
            vka_cspace_make_path(vka, cap, &path);
            assert(vka_cnode_delete(&path) == 0);
            vka_cspace_free(vka, cap);
        }
    };
    vmap->caps_.clear(free_func);

    /* Remove from vmap list, delete vmap */
    map_list_.remove(vmap);
    delete vmap;
}

/* TODO cache attributes */
bool ZxVmo::commit_page(uint32_t index, VmoMapping *vmap)
{
    int err;
    vka_t *vka = get_server_vka();
    assert(index < num_pages_);

    /* Ensure index backed in page array */
    if (!frames_.alloc(index)) {
        return false;
    }

    /* If frame not yet allocated, alloc & map into server */
    if (frames_[index].cptr == 0) {
        /* Allocate a frame object */
        /* TODO at paddr, maybe device? */
        err = vka_alloc_frame(vka, seL4_PageBits, &frames_[index]);
        if (err) {
            return false;
        }

        /* Map frame into kmap. We leak backing PTs for server */
        uintptr_t kvaddr = kaddr_ + (index * (1 << seL4_PageBits));
        err = sel4utils_map_page_leaky(vka, seL4_CapInitThreadVSpace,
                frames_[index].cptr, (void *)kvaddr, seL4_AllRights, 1);
        if (err) {
            vka_free_object(vka, &frames_[index]);
            memset(&frames_[index], 0, sizeof(vka_object_t));
            return false;
        }
    }

    /* If a vmap was supplied, map into its proc's addrspace */
    if (vmap != NULL) {
        assert(map_list_.contains(vmap));
        assert(frames_[index].cptr != 0);

        /* Check this index lies in mapping */
        uint32_t vmap_end_page = vmap->start_page_ + vmap->num_pages_;
        if (index < vmap->start_page_ || index >= vmap_end_page) {
            return false;
        }

        /* check for backing */
        if (!vmap->caps_.alloc(index)) {
            return false;
        }

        if (vmap->caps_[index] == 0) {
            /* src is kmap slot, dest is vmap slot */
            cspacepath_t src, dest;
            err = vka_cspace_alloc_path(vka, &dest);
            if (err) {
                return false;
            }
            vka_cspace_make_path(vka, frames_[index].cptr, &src);

            /* Copy to dest slot */
            err = vka_cnode_copy(&dest, &src, seL4_AllRights);
            if (err) {
                vka_cspace_free_path(vka, dest);
                return false;
            }

            /* Map into proc addrspace */
            ZxProcess *proc = vmap->parent_->get_proc();
            uintptr_t vaddr = vmap->base_addr_ + ((index - vmap->start_page_) * vmoPageSize);
            //dprintf(SPEW, "Mapping page at %lx for proc %s\n", vaddr, proc->get_name());
            err = proc->map_page_in_vspace(dest.capPtr, (void *)vaddr, vmap->rights_, 1);
            if (err) {
                vka_cnode_delete(&dest);
                vka_cspace_free_path(vka, dest);
                return false;
            }

            /* Set vmap cap */
            vmap->caps_[index] = dest.capPtr;
        }
    }

    return true;
}

void ZxVmo::decommit_page(uint32_t index)
{
    vka_t *vka = get_server_vka();
    assert(index < num_pages_);

    /* For each mapping, unmap page and delete cap */
    auto unmap_func = [&vka, &index] (VmoMapping *vmap) {
        if (vmap->caps_.has(index) && vmap->caps_[index] != 0) {
            cspacepath_t path;
            vka_cspace_make_path(vka, vmap->caps_[index], &path);
            seL4_ARCH_Page_Unmap(vmap->caps_[index]);
            vka_cnode_delete(&path);
            vka_cspace_free_path(vka, path);
            /* Reset cptr */
            vmap->caps_[index] = 0;
        }
    };
    map_list_.for_each(unmap_func);

    /* Unmap page from kmap and free frame object */
    if (frames_.has(index) && frames_[index].cptr != 0) {
        seL4_ARCH_Page_Unmap(frames_[index].cptr);
        vka_free_object(vka, &frames_[index]);
        /* Reset frame object */
        memset(&frames_[index], 0, sizeof(vka_object_t));
    }
}

zx_status_t ZxVmo::resize(size_t new_num_pages)
{
    if (new_num_pages == num_pages_) {
        return ZX_OK;
    }

    /* To resize, we create a new page array, and try to copy
       all objs from the old one. For now, we only support this
       when no user mappings currently exist. */
    if (map_list_.size() > 0) {
        return ZX_ERR_NOT_SUPPORTED;
    }

    /* Make a new page array */
    PageArray<vka_object_t> new_pa;
    if (!new_pa.init(new_num_pages)) {
        return ZX_ERR_NO_MEMORY;
    }

    /* Copy over old vka objects */
    size_t to_copy = (new_num_pages < num_pages_) ? new_num_pages : num_pages_;
    for (size_t i = 0; i < to_copy; ++i) {
        if (frames_.has(i) && frames_[i].cptr != 0) {
            if (!new_pa.alloc(i)) {
                /* Not enough pages for resize! Clear the new pagearray.
                   We don't need to do any internal cleanup. */
                new_pa.clear([] (vka_object_t &frame) {});
                return ZX_ERR_NO_MEMORY;
            }
            /* Copy the frame vka object */
            new_pa[i] = frames_[i];
        }
    }

    /* If we are shrinking vmo, free the vka objects now
       out of range */
    if (new_num_pages < num_pages_) {
        for (size_t i = new_num_pages; i < num_pages_; ++i) {
            decommit_page(i);
        }
    }

    /* Resize successful. Swap the pagearrays, and update size params */
    frames_.swap(new_pa);
    num_pages_ = new_num_pages;
    size_ = new_num_pages * (1 << seL4_PageBits);

    return ZX_OK;
}
