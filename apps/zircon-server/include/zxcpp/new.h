#pragma once

#include <stdlib.h>

/* 
 * Note: All calls to new should use preallocated memory.
 * We want to know if we have memory available before attempting
 * to create an object.
 */

void* operator new(size_t , void *p)
{
    return p;
}

void* operator new[](size_t , void* p)
{
    return p;
}

void operator delete(void *p)
{
    return ::free(p);
}

void operator delete[](void *p)
{
    return ::free(p);
}

void operator delete(void *p, size_t s) {
    return ::free(p);
}

void operator delete[](void *p, size_t s) {
    return ::free(p);
}
