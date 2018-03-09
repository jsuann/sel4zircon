#pragma once

#include <autoconf.h>

#include "listable.h"

/* Template for managing listable objects */

template <typename T> 
class LinkedList {
public:
    LinkedList(ZxObject *owner) : head_{NULL}, tail_{NULL},
            num_items_{0}, owner_{owner} {}

    /* Assume list will be empty before destruction */
    ~LinkedList() {}

    bool empty() const {
        return (num_items_ == 0);
    }

    T *front() const {
        return head_;
    }

    T *back() const {
        return tail_;
    }

    int size() const {
        return num_items_;
    }

    void push_back(T *item) {
        if (num_items_ == 0) {
            head_ = item;
            tail_ = item;
        } else {
            tail_->set_next(item);
            item->set_prev(tail_);
            tail_ = item;
        }
        ++num_items_;
    }

    void push_front(T *item) {
        if (num_items_ == 0) {
            head_ = item;
            tail_ = item;
        } else {
            head_->set_prev(item);
            item->set_next(head_);
            head_ = item;
        }
        ++num_items_;
    }

    T *pop_back() {
        T *item = tail_;
        tail_ = item->get_prev();
        --num_items_;
        if (num_items_ > 0) {
            tail_->set_next(NULL);
        }
        item->set_next(NULL);
        item->set_prev(NULL);
        return item;
    }

    T *pop_front() {
        T *item = head_;
        head_ = item->get_next();
        --num_items_;
        if (num_items_ > 0) {
            head_->set_prev(NULL);
        }
        item->set_next(NULL);
        item->set_prev(NULL);
        return item;
    }

    bool remove(T *item) {
        if (item->get_owner() == owner_) {
            T *prev = item->get_prev();
            T *next = item->get_next();
            if (prev != NULL) {
                prev->set_next(item->next);
            }
            if (next != NULL) {
                next->set_prev(item->prev);
            }
            item->set_next(NULL);
            item->set_prev(NULL);
            --num_items_;
            return true;
        }
        return false;
    }

private:
    T *head_;
    T *tail_;
    int num_items_;
    /* Sanity checking: linked lists are linked to an owning object */
    ZxObject *owner_;
};
