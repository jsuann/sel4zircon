#include "object/object.h"

zx_koid_t global_koid = 1024ULL;

ZxObject::ZxObject() : koid_{global_koid},  handle_count_{0}, signals_{0}
{
    ++global_koid;
}
