#pragma once

#include <autoconf.h>

#include "object.h"

/* Wrapper class for objects used in linked lists */
/* Must have a single owner, or a pointer wrapper */

class Listable {
public:
    Listable() : next_{NULL}, prev_{NULL} {};
    virtual ~Listable() {};

    virtual ZxObject *get_owner() const {
        return NULL;
    }

    Listable *get_next() const {
        return next_;
    }

    Listable *get_prev() const {
        return prev_;
    }

    void set_next(Listable *next) {
        next_ = next;
    }

    void set_prev(Listable *prev) {
        prev_ = prev;
    }

private:
    Listable *next_;
    Listable *prev_;
};
