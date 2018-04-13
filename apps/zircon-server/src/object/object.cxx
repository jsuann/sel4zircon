#include "object/object.h"

namespace ObjectCxx {

zx_koid_t global_koid = 1024ULL;

/* Helper struct for determining if object is of a required type */
template <typename T> struct ZxObjectType;

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

#define DECL_OBJ_TYPE(T, E) \
class T; \
namespace ObjectCxx { \
template <> struct ZxObjectType<T> { \
    static constexpr zx_obj_type_t ID = E; \
}; \
}

DECL_OBJ_TYPE(ZxProcess, ZX_OBJ_TYPE_PROCESS)
DECL_OBJ_TYPE(ZxVmo, ZX_OBJ_TYPE_VMO)

#undef DECL_OBJ_TYPE

template <typename T>
bool is_object_type(ZxObject *obj)
{
    return (obj->get_object_type() == ObjectCxx::ZxObjectType<T>::ID);
}
