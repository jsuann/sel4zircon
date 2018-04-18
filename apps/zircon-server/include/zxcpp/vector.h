#pragma once

#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

constexpr size_t vectorBaseSize = 4;

template <typename T> bool cmp(T a, T b);

template <typename T>
class Vector {
private:
    bool resize() {
        size_t new_size = (size_ == 0) ? vectorBaseSize : (size_ * 2);
        T *mem = (T *)malloc(new_size * sizeof(T));
        if (mem == NULL) {
            return false;
        }
        memmove(mem, vec_, (size_ * sizeof(T)));
        free(vec_);
        vec_ = mem;
        size_ = new_size;
        return true;
    }

public:
    Vector() = default;
    ~Vector() {
        free(vec_);
    }

    size_t size() {
        return num_items_;
    }

    T *get() {
        return vec_;
    }

    bool contains(T item) {
        for (size_t i = 0; i < num_items_; ++i) {
            if (vec_[i] == item) {
                return true;
            }
        }
        return false;
    }

    bool insert(T item) {
        /* Resize if needed */
        if ((num_items_ + 1) > size_) {
            if (!resize()) {
                return false;
            }
        }

        /* Find insert location */
        size_t i = 0;
        for (; i < num_items_; ++i) {
            if (cmp(item, vec_[i])) {
                break;
            }
        }
        assert(i <= num_items_);

        /* Make space for item at i */
        memmove((&vec_[i] + 1), &vec_[i], ((num_items_ - i) * sizeof(T)));

        /* Insert item at i */
        vec_[i] = item;
        ++num_items_;
        return true;
    }

    bool remove(T item) {
        /* Find index of item */
        size_t i = 0;
        for (; i < num_items_; ++i) {
            if (item == vec_[i]) {
                break;
            }
        }
        if (i == num_items_) {
            return false;
        }

        /* Memmove over item */
        memmove(&vec_[i], (&vec_[i] + 1), ((num_items_ - i) * sizeof(T)));
        --num_items_;
        return true;
    }

    void clear() {
        free(vec_);
        size_ = 0;
        num_items_ = 0;
        /* vec must be NULL so destructor doesn't double free */
        vec_ = NULL;
    }

private:
    T *vec_ = NULL;
    size_t size_ = 0;
    size_t num_items_ = 0;
};
