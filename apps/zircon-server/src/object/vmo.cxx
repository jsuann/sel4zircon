#include "object/vmo.h"
#include "object/process.h"

bool ZxVmo::init()
{
    /* Create array of uninitialised frame objects */
    frames_ = calloc(num_pages_, sizeof(vka_object_t));
    if (frames_ == NULL) {
        return false;
    }
}

void ZxVmo::destroy()
{
    vka_t *vka = get_server_vka();

    /* Free any allocated frame objects */
    for (size_t i = 0; i < num_pages_; ++i) {
        if (frames_[i].cptr != 0) {
            vka_free_object(vka, &frames_[i]);
        }
    }

    /* Free frame array */
    free(frames_);
}

VmoMapping *ZxVmo::create_mapping(uintptr_t start_addr, ZxVmar *vmar)
{
    /* Alloc mem for vmap */
    void *vmap_mem = malloc(sizeof(VmoMapping));
    if (vmap_mem == NULL) {
        return NULL;
    }

    /* Alloc mem for cap array */
    seL4_CPtr *caps = calloc(num_pages_, sizeof(seL4_CPtr));
    if (caps == NULL) {
        free(vmap_mem);
        return NULL;
    }

    VmoMapping *vmap = new (vmap_mem) VmoMapping(start_addr, caps, vmar);
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
bool commit_page(uint32_t index, VmoMapping *vmap, uint32_t flags)
{
    vka_t *vka = get_server_vka();
    assert(index < num_pages_);

    /* If frame not yet allocated, alloc & map into server */
    if (frames_[index].cptr == 0) {
        /* TODO at paddr, maybe device? */
        int err = vka_alloc_frame(vka, seL4_PageBits, &frames_[index]);
        if (err) {
            return false;
        }
        uintptr_t kvaddr = kaddr_ + (index * (1 << seL4_PageBits));
        /* vmo kmap is likely to be reused so leak PTs */
        int err = sel4utils_map_page_leaky(vka, seL4_CapInitThreadVSpace,
                frames_[index].cptr, (void *)kvaddr, seL4_AllRights, 1);
        if (err) {
            /* TODO delete frame */
            return false;
        }
    }

    /* If a vmap was supplied, map into its proc's addrspace */
    if (vmap != NULL) {
        assert(map_list_.contains(vmap));
        assert(frames_[index].cptr != 0);
        if (vmap->caps_[index] != 0) {
            cspacepath_t src, dest;
            /* src is kmap slot, dest is vmap slot */
            int err = vka_cspace_alloc(vka, &dest);
            if (err) {
                return false;
            }
            vka_cspace_make_path(vka, frames_[index].cptr, &dest);
        }
    }

    return true;
}
