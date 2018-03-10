#pragma once

#include <autoconf.h>

/* Wrapper class for objects used in linked lists */
/* Must have a single owner, or a pointer wrapper */

class ZxObject;

template <typename T>
class Listable {
public:
    Listable() : next_{NULL}, prev_{NULL} {};
    virtual ~Listable() {};

    virtual ZxObject *get_owner() const {
        return NULL;
    }

    virtual void set_owner(ZxObject *) {}

    T *get_next() const {
        return next_;
    }

    T *get_prev() const {
        return prev_;
    }

    void set_next(T *next) {
        next_ = next;
    }

    void set_prev(T *prev) {
        prev_ = prev;
    }

private:
    T *next_;
    T *prev_;
};
