#include "object/object.h"

namespace ObjectCxx {

zx_koid_t global_koid = 1024ULL;

}

ZxObject::ZxObject() : koid_{ObjectCxx::global_koid},
    handle_count_{0}, signals_{0}
{
    ++ObjectCxx::global_koid;
}

template <typename T, typename ... U>
T *allocate_object(U ... args)
{
    void *p = malloc(sizeof(T));
    if (p == NULL) {
        return NULL;
    }
    return new (p) T(args...);
}

template <typename T>
void free_object(T *obj)
{
    delete obj;
}

void destroy_object(ZxObject *obj)
{
    zx_obj_type_t type = obj->get_object_type();
    assert(type != ZX_OBJ_TYPE_NONE);
    switch (type) {
    case ZX_OBJ_TYPE_PROCESS:
        free_object<ZxProcess>((ZxProcess *)obj);
        break;
    default:
        free_object(obj);
    }
}
