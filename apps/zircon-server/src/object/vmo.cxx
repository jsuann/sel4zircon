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

    /* Create array of uninitialised frame objects */
    frames_ = (vka_object_t *)calloc(num_pages_, sizeof(vka_object_t));
    return (frames_ != NULL);
}

void ZxVmo::destroy()
{
    vka_t *vka = get_server_vka();

    /* TODO Destroy vmo mappings: remove from vmar, then delete */

    /* Free any allocated frame objects */
    if (frames_ != NULL) {
        for (size_t i = 0; i < num_pages_; ++i) {
            if (frames_[i].cptr != 0) {
                vka_free_object(vka, &frames_[i]);
            }
        }
    }

    /* Free frame array */
    free(frames_);

    /* Free kmap */
    if (kaddr_ != 0) {
        free_vmo_kmap(kaddr_);
    }
}

VmoMapping *ZxVmo::create_mapping(uintptr_t start_addr, ZxVmar *vmar, uint32_t flags)
{
    /* Alloc mem for vmap */
    void *vmap_mem = malloc(sizeof(VmoMapping));
    if (vmap_mem == NULL) {
        return NULL;
    }

    /* Alloc mem for cap array */
    seL4_CPtr *caps = (seL4_CPtr *)calloc(num_pages_, sizeof(seL4_CPtr));
    if (caps == NULL) {
        free(vmap_mem);
        return NULL;
    }

    /* get frame access rights from mapping flags (should be valid by this point!) */
    seL4_CapRights_t rights;
    bool can_read = (flags & ZX_VM_FLAG_PERM_READ || flags & ZX_VM_FLAG_PERM_EXECUTE);
    bool can_write = (flags & ZX_VM_FLAG_PERM_WRITE);
    rights = seL4_CapRights_new(0, can_read, can_write);

    /* Create mapping */
    VmoMapping *vmap = new (vmap_mem) VmoMapping(start_addr, caps, rights, vmar);
    map_list_.push_back(vmap);
    return vmap;
}

void ZxVmo::delete_mapping(VmoMapping *vmap)
{
    vka_t *vka = get_server_vka();
    assert(map_list_.contains(vmap));

    /* Unmap all caps, free cap array */
    for (uint32_t i = 0; i < num_pages_; ++i) {
        if (vmap->caps_[i] != 0) {
            cspacepath_t path;
            vka_cspace_make_path(vka, vmap->caps_[i], &path);
            assert(vka_cnode_delete(&path) == 0);
            vka_cspace_free(vka, vmap->caps_[i]);
        }
    }
    free(vmap->caps_);

    /* Remove from vmap list, delete vmap */
    map_list_.remove(vmap);
    delete vmap;
}

/* TODO cache attributes? */
bool ZxVmo::commit_page(uint32_t index, VmoMapping *vmap)
{
    int err;
    vka_t *vka = get_server_vka();
    assert(index < num_pages_);

    /* If frame not yet allocated, alloc & map into server */
    if (frames_[index].cptr == 0) {
        /* TODO at paddr, maybe device? */
        err = vka_alloc_frame(vka, seL4_PageBits, &frames_[index]);
        if (err) {
            return false;
        }
        uintptr_t kvaddr = kaddr_ + (index * (1 << seL4_PageBits));
        /* vmo kmap is likely to be reused so leak PTs */
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
        if (vmap->caps_[index] == 0) {
            cspacepath_t src, dest;
            /* src is kmap slot, dest is vmap slot */
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
            uintptr_t vaddr = vmap->start_addr_ + (index * (1 << seL4_PageBits));
            dprintf(INFO, "Mapping page at %lx for proc %p\n", vaddr, proc);
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
        if (vmap->caps_[index] != 0) {
            cspacepath_t path;
            vka_cspace_make_path(vka, vmap->caps_[index], &path);
            seL4_X86_Page_Unmap(vmap->caps_[index]);
            vka_cnode_delete(&path);
            vka_cspace_free_path(vka, path);
            /* Reset cptr */
            vmap->caps_[index] = 0;
        }
    };
    map_list_.for_each(unmap_func);

    /* Unmap page from kmap and free frame object */
    if (frames_[index].cptr != 0) {
        seL4_X86_Page_Unmap(frames_[index].cptr);
        vka_free_object(vka, &frames_[index]);
        /* Reset frame object */
        memset(&frames_[index], 0, sizeof(vka_object_t));
    }
}
