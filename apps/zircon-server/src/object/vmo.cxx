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
    void *mem = malloc(sizeof(VmoMapping));
    if (mem == NULL) {
        return NULL;
    }

    VmoMapping *vmap = new (mem) VmoMapping(start_addr, vmar);
    map_list_.push_back(vmap);
    ++num_mappings_;
    return vmap;
}

bool commit_page(uint32_t index, VmoMapping *vmap)
{
    vka_t *vka = get_server_vka();
    assert(index < num_pages_);

    /* If frame not yet allocated, alloc & map into server */
    if (frames_[index].cptr == 0) {
        /* TODO at paddr, maybe device? */
        vka_alloc_frame(vka, seL4_PageBits, &frames_[index]);
        if (frames_[index].cptr == 0) {
            return false;
        }
        uintptr_t kvaddr = kaddr_ + (index * (1 << seL4_PageBits));
        /* vmo kmap is likely to be reused so leak PTs */
        //sel4utils_map_page_leaky(vka,
    }

    return true;
}
