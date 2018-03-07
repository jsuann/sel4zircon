#include <autoconf.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>

extern "C" {
#include <sel4/sel4.h>
#include <sel4utils/process.h>
#include <zircon/types.h>
#include <zircon/syscalls.h>
}

extern "C" void test_cpp(void);

void* operator new(size_t s) noexcept {
    if (s == 0u) {
        s = 1u;
    }
    return ::malloc(s);
}

void* operator new[](size_t s) noexcept {
    if (s == 0u) {
        s = 1u;
    }
    return ::malloc(s);
}

void* operator new(size_t , void *p) {
    return p;
}

void* operator new[](size_t , void* p) {
    return p;
}

void operator delete(void *p) {
    return ::free(p);
}

void operator delete[](void *p) {
    return ::free(p);
}

void operator delete(void *p, size_t s) {
    return ::free(p);
}

void operator delete[](void *p, size_t s) {
    return ::free(p);
}

class Tester {
public:
    Tester() : x{99999} {};
    void y() { printf("%d\n", x); };
private:
    int x;
};

void test_cpp(void)
{
    Tester t;
    t.y();
    int *n = new int;
    *n = 888;
    printf("n: %p %d\n", n, *n);
    delete n;
}
