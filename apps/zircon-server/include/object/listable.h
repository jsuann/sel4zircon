#pragma once

#include <autoconf.h>

/* Wrapper class for objects used in linked lists */

class Listable {
public:
    Listable() = default;
    virtual ~Listable() {};

    Listable *get_next() const {
        return next_;
    }

    Listable *get_prev() const {
        return prev_;
    }

    void set_next(Listable *next) {
        next_ = next;
    }

    void set_prev(Listable *next) {
        prev_ = prev;
    }
private:
    Listable *next_;
    Listable *prev_;
}
