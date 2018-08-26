#pragma once

#include <autoconf.h>

#include <assert.h>

extern "C" {
#include "../debug.h"
}

#include "listable.h"

/* Template for managing listable objects */

/* Set to 1 if we need to make sure the list is in a sane state */
#define ZX_LL_SANITY_CHECK  1

template <typename T>
class LinkedList {
public:
    LinkedList(ZxObject *owner) : head_{NULL}, tail_{NULL},
        num_items_{0}, list_owner_{owner} {
#if ZX_LL_SANITY_CHECK
        assert(list_owner_ != NULL);
#endif
    }

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

    size_t size() const {
        return num_items_;
    }

    bool contains(T *item) {
        return (item->owner_ == list_owner_);
    }

    void push_back(T *item) {
#if ZX_LL_SANITY_CHECK
        sanity_check(item, false);
#endif

        if (num_items_ == 0) {
            head_ = item;
            tail_ = item;
        } else {
            tail_->next_ = item;
            item->prev_ = tail_;
            tail_ = item;
        }

        ++num_items_;
        item->owner_ = list_owner_;
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
            head_->prev_ = item;
            item->next_ = head_;
            head_ = item;
        }

        ++num_items_;
        item->owner_ = list_owner_;
#if ZX_LL_SANITY_CHECK
        sanity_check(item, true);
#endif
    }

    T *pop_back() {
        T *item = tail_;
#if ZX_LL_SANITY_CHECK
        sanity_check(item, true);
#endif
        tail_ = item->prev_;
        --num_items_;

        if (num_items_ > 0) {
            tail_->next_ = NULL;
        } else {
            head_ = NULL;
        }

        item->next_ = NULL;
        item->prev_ = NULL;
        item->owner_ = NULL;
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
        head_ = item->next_;
        --num_items_;

        if (num_items_ > 0) {
            head_->prev_ = NULL;
        } else {
            tail_ = NULL;
        }

        item->next_ = NULL;
        item->prev_ = NULL;
        item->owner_ = NULL;
#if ZX_LL_SANITY_CHECK
        sanity_check(item, false);
#endif
        return item;
    }

    void remove(T *item) {
#if ZX_LL_SANITY_CHECK
        sanity_check(item, true);
#endif
        T *prev = item->prev_;
        T *next = item->next_;

        if (prev != NULL) {
            prev->next_ = item->next_;
        }

        if (next != NULL) {
            next->prev_ = item->prev_;
        }

        if (head_ == item) {
            head_ = item->next_;
        }

        if (tail_ == item) {
            tail_ = item->prev_;
        }

        item->next_ = NULL;
        item->prev_ = NULL;
        item->owner_ = NULL;
        --num_items_;
#if ZX_LL_SANITY_CHECK
        sanity_check(item, false);
#endif
    }

    template <typename F, typename ... U>
    void for_each(F &&func, U ... args) {
        T *item = head_;

        while (item != NULL) {
            /* Save a ref to next before func call to
               allow item to remove itself from list */
            T *next = item->next_;
            func(item, args...);
            item = next;
        }
    }

#if ZX_LL_SANITY_CHECK
    void sanity_check(T *item, bool in_list) {
        dprintf(SPEW, "LL sanity check on %p, in list: %s\n",
                item, ((in_list) ? "true" : "false"));

        /* Check ownership of item */
        if (in_list) {
            assert(item->owner_ == list_owner_);
        } else {
            assert(item->owner_ == NULL);
        }

        /* Check forward */
        size_t item_count = 0;
        size_t i = 0;
        T *t = head_;

        while (t != NULL) {
            assert(t->owner_ == list_owner_);

            if (!in_list) {
                assert(t != item);
            } else if (t == item) {
                ++item_count;
            }

            ++i;

            if (i > num_items_) {
                break;
            }

            t = t->next_;
        }

        assert(i == num_items_);

        if (in_list) {
            assert(item_count == 1);
        }

        /* Check backward */
        i = item_count = 0;
        t = tail_;

        while (t != NULL) {
            assert(t->owner_ == list_owner_);

            if (!in_list) {
                assert(t != item);
            } else if (t == item) {
                ++item_count;
            }

            ++i;

            if (i > num_items_) {
                break;
            }

            t = t->prev_;
        }

        assert(i == num_items_);

        if (in_list) {
            assert(item_count == 1);
        }
    }
#endif

private:
    T *head_;
    T *tail_;
    size_t num_items_;
    /* Linked lists have a single owning object */
    ZxObject *const list_owner_;
};
