#pragma once

#include <autoconf.h>

#include <assert.h>

#include "listable.h"

/* Template for managing listable objects */

/* Set to 1 if we need to make sure the list is in a sane state */
#define ZX_LL_SANITY_CHECK  1

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

    bool contains(T *item) {
        return (item->get_owner() == owner_);
    }

    void push_back(T *item) {
#if ZX_LL_SANITY_CHECK
        sanity_check(item, false);
#endif
        if (num_items_ == 0) {
            head_ = item;
            tail_ = item;
        } else {
            tail_->set_next(item);
            item->set_prev(tail_);
            tail_ = item;
        }
        ++num_items_;
#if ZX_LL_SANITY_CHECK
        sanity_check(item, true);
#endif
    }

    void push_front(T *item) {
#if ZX_LL_SANITY_CHECK
        sanity_check(item, false);
#endif
        if (num_items_ == 0) {
            head_ = item;
            tail_ = item;
        } else {
            head_->set_prev(item);
            item->set_next(head_);
            head_ = item;
        }
        ++num_items_;
#if ZX_LL_SANITY_CHECK
        sanity_check(item, true);
#endif
    }

    T *pop_back() {
        T *item = tail_;
#if ZX_LL_SANITY_CHECK
        sanity_check(item, true);
#endif
        tail_ = item->get_prev();
        --num_items_;
        if (num_items_ > 0) {
            tail_->set_next(NULL);
        }
        item->set_next(NULL);
        item->set_prev(NULL);
#if ZX_LL_SANITY_CHECK
        sanity_check(item, false);
#endif
        return item;
    }

    T *pop_front() {
        T *item = head_;
#if ZX_LL_SANITY_CHECK
        sanity_check(item, true);
#endif
        head_ = item->get_next();
        --num_items_;
        if (num_items_ > 0) {
            head_->set_prev(NULL);
        }
        item->set_next(NULL);
        item->set_prev(NULL);
#if ZX_LL_SANITY_CHECK
        sanity_check(item, false);
#endif
        return item;
    }

    void remove(T *item) {
#if ZX_LL_SANITY_CHECK
        sanity_check(item, true);
#endif
        T *prev = item->get_prev();
        T *next = item->get_next();
        if (prev != NULL) {
            prev->set_next(item->get_next());
        }
        if (next != NULL) {
            next->set_prev(item->get_prev());
        }
        if (head_ == item) {
            head_ = item->get_next();
        }
        if (tail_ == item) {
            tail_ = item->get_prev();
        }
        item->set_next(NULL);
        item->set_prev(NULL);
        --num_items_;
#if ZX_LL_SANITY_CHECK
        sanity_check(item, false);
#endif
    }

    template <typename F, typename ... U>
    void for_each(F &&func, U ... args) {
        T *item = head_;
        while (item != NULL) {
            func(item, args...);
            item = item->get_next();
        }
    }

#if ZX_LL_SANITY_CHECK
    void sanity_check(T *item, bool in_list) {
        dprintf(INFO, "LL sanity check on %p, in list: %s\n",
                item, ((in_list) ? "true" : "false"));
        int item_count = 0;
        /* Check forward */
        int i = 0;
        T *t = head_;
        while (t != NULL) {
            assert(t->get_owner() == owner_);
            if (!in_list)
                assert(t != item);
            else if (t == item)
                ++item_count;
            ++i;
            if (i > num_items_)
                break;
            t = t->get_next();
        }
        assert(i == num_items_);
        if (in_list)
            assert(item_count == 1);

        /* Check backward */
        i = item_count = 0;
        t = tail_;
        while (t != NULL) {
            assert(t->get_owner() == owner_);
            if (!in_list)
                assert(t != item);
            else if (t == item)
                ++item_count;
            ++i;
            if (i > num_items_)
                break;
            t = t->get_prev();
        }
        assert(i == num_items_);
        if (in_list)
            assert(item_count == 1);
    }
#endif

private:
    T *head_;
    T *tail_;
    int num_items_;
    /* Sanity checking: linked lists are linked to an owning object */
    ZxObject * const owner_;
};
