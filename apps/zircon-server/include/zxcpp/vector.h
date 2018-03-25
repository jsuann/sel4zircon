#pragma once

#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

constexpr size_t vectorBaseSize = 4;

template <typename T> bool cmp(T *a, T *b);

template <typename T>
class Vector {
private:
    bool resize() {
        size_t new_size = (size_ == 0) ? vectorBaseSize : (size_ * 2);
        T *mem = malloc(new_size * sizeof(T));
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
            if (cmp(item,vec[i])) {
                break;
            }
        }
        assert(i <= num_items_);

        /* Make space for item at i */
        memmove((&vec[i] + 1), &vec[i], ((num_items_ - i) * sizeof(T)));

        /* Insert item at i */
        vec[i] = item;
        ++num_items_;
        return true;
    }

    bool remove(T item) {
        /* Find index of item */
        size_t i = 0;
        for (; i < num_items_; ++i) {
            if (item == vec[i]) {
                break;
            }
        }
        if (i == num_items_) {
            return false;
        }
        
        /* Memmove over item */
        memmove(&vec[i], (&vec[i] + 1), ((num_items_ - i) * sizeof(T)));
        --num_items_;
        return true;
    }

private:
    T *vec_ = NULL;
    size_t size_ = 0;
    size_t num_items_ = 0;
};
