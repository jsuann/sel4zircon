#include "object/object.h"

namespace ObjectCxx {

zx_koid_t global_koid = 1024ULL;

/* Helper struct for determining if object is of a required type */
template <typename T> struct ZxObjectType;

} /* namespace ObjectCxx */

/* Helper class to get default object values */
#define DECL_OBJ_TYPE(T, E, R) \
class T; \
namespace ObjectCxx { \
template <> struct ZxObjectType<T> { \
    static constexpr zx_obj_type_t ID = E; \
    static constexpr zx_rights_t RIGHTS = R; \
}; \
}

DECL_OBJ_TYPE(ZxProcess, ZX_OBJ_TYPE_PROCESS, ZX_DEFAULT_PROCESS_RIGHTS)
DECL_OBJ_TYPE(ZxThread, ZX_OBJ_TYPE_THREAD, ZX_DEFAULT_THREAD_RIGHTS)
DECL_OBJ_TYPE(ZxVmo, ZX_OBJ_TYPE_VMO, ZX_DEFAULT_VMO_RIGHTS)
DECL_OBJ_TYPE(ZxVmar, ZX_OBJ_TYPE_VMAR, ZX_DEFAULT_VMAR_RIGHTS)

#undef DECL_OBJ_TYPE

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

/* Object destroyer upon all handles closed */
void destroy_object(ZxObject *obj)
{
    /* XXX sanity check */
    assert(obj->can_destroy());

    /* Perform any internal destruction in object */
    obj->destroy();

    /* Free object memory */
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

void destroy_handle_maybe_object(Handle *h)
{
    ZxObject *o = h->get_object();
    o->destroy_handle(h);
    if (o->can_destroy()) {
        /* Try to destroy object */
        destroy_object(o);
    }
}

template <typename T>
bool is_object_type(ZxObject *obj)
{
    return (obj->get_object_type() == ObjectCxx::ZxObjectType<T>::ID);
}

template <typename T>
Handle *create_handle_default_rights(T *obj)
{
    zx_rights_t default_rights = ObjectCxx::ZxObjectType<T>::RIGHTS;
    return obj->create_handle(default_rights);
}
