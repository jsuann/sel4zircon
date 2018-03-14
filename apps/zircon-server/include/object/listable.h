#pragma once

#include <autoconf.h>

/* Wrapper class for objects used in linked lists */
/* Must have a single owner, or a pointer wrapper */

class ZxObject;
template <typename T> class LinkedList;

template <typename T>
class Listable {
    friend LinkedList<T>;
public:
    Listable() : next_{NULL}, prev_{NULL}, owner_{NULL} {};
    virtual ~Listable() {};

    ZxObject *get_owner() const {
        return owner_;
    }

private:
    T *next_;
    T *prev_;
    ZxObject *owner_;
};
