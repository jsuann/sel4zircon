#pragma once

#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

constexpr size_t stackAllocMaxNumElems = (1 << 31);

template <typename T>
class StackAlloc {
private:
    struct FreeListNode {
        uint32_t next       : 31;
        uint32_t is_free    :  1;
    };

public:
    StackAlloc() = default;
    ~StackAlloc() {}

    bool init(void *pool, size_t count) {
        if (count > stackAllocMaxNumElems) {
            return false;
        }
        free_list_ = (FreeListNode *)malloc(sizeof(FreeListNode) * count);
        if (free_list_ == NULL) {
            return false;
        }
        for (size_t i = 0; i < count - 1; ++i) {
            free_list_[i].next = i+1;
            free_list_[i].is_free = 1;
        }
        free_list_[count-1].next = 0;
        free_list_[count-1].is_free = 1;

        /* init other vals */
        pool_ = (T *)pool;
        count_ = count;
        num_alloc_ = 0;
        next_free_ = 0;
        return true;
    }

    void destroy() {
        /* Make sure we don't use StackAlloc's free! */
        ::free(free_list_);
    }

    bool alloc(uint32_t &index) {
        if (num_alloc_ == count_) {
            return false;
        }
        index = next_free_;
        //assert(free_list_[index].is_free);
        free_list_[index].is_free = 0;
        next_free_ = free_list_[index].next;
        ++num_alloc_;
        return true;
    }

    bool is_alloc(uint32_t index) {
        return (free_list_[index].is_free == 0);
    }

    T *get(uint32_t index) {
        assert(index < stackAllocMaxNumElems);
        assert(!free_list_[index].is_free);
        return pool_ + index;
    }

    void free(uint32_t index) {
        assert(index < stackAllocMaxNumElems);
        assert(!free_list_[index].is_free);
        free_list_[index].next = next_free_;
        free_list_[index].is_free = 1;
        next_free_ = index;
        --num_alloc_;
    }

    uint32_t num_avail() const {
        return count_ - num_alloc_;
    }

private:
    T *pool_;
    size_t count_;
    uint32_t next_free_;
    uint32_t num_alloc_;
    FreeListNode *free_list_ = NULL;
};
