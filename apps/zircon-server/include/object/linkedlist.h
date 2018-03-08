#pragma once

#include <autoconf.h>

/* Template for managing listable objects */

template <typename T> class LinkedList;

template <typename T> 
class LinkedList<T> {
public:
    LinkedList() : head_{NULL}, tail_{NULL}, num_items{0} {}
private:
    T *head_;
    T *tail_;
    int num_items_;
};
