/* Simple refptr implementation */
#include <stdlib.h>

template <typename T>
class RefPtr {
public:
    /* ref count of -1 means ptr uninitialized */
    RefPtr<T>() : ptr{NULL}, refCount{NULL} {}
    RefPtr<T>(T *_ptr) : ptr{_ptr} {
        refCount = malloc(sizeof(int)); 
        *refCount = 0;
    }

    /* Copy constructor / assignment */
    RefPtr(const RefPtr<t> &orig) {

    }
private:
    T *ptr;
    int *refCount;
};
