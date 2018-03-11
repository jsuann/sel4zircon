#pragma once

#include <autoconf.h>
#include <stdlib.h>

constexpr stackAllocMaxNumElems = (1 << 31);

template <typename T>
class StackAlloc {
private:
    struct FreeListNode {
        uint32_t next       : 31;
        uint32_t is_free    :  1;
    };

public:
    StackAlloc() = default;
    ~StackAlloc();

    bool init(T *pool, size_t count) {
        if (count > stackAllocMaxNumElems) {
            return false;
        }
        free_list_ = malloc(sizeof(struct FreeListNode) * count);
        if (free_list_ == NULL) {
            return false;
        }
        for (int i = 0; i < count - 1; i++) {
            free_list_[i].next = i+1;
            free_list_[i].is_free = 1;
        }
        free_list_[count-1].next = 0;
        free_list_[count-1].is_free = 1;

        /* init other vals */
        pool_ = pool;
        count_ = count;
        num_alloc_ = 0;
        next_free_ = 0;
    }

    void destroy() {
        free(free_list_);
    }

    bool alloc(uint32_t &index) {
        if (num_alloc_ == count_) {
            return false;
        }
        index = next_free_;
        assert(free_list_[index].is_free);
        free_list_[index].is_free = 0;
        next_free_ = free_list_[index].next;
        ++num_alloc_;
        return true;
    }

    T *get(uint32_t index) {
        assert(index < stackAllocMaxNumElems);
        assert(!free_list_[index].is_free);
        return &pool_[index];
    }

    void free(uint32_t index) {
        assert(index < stackAllocMaxNumElems);
        assert(!free_list_[index].is_free);
        free_list_[index].next = next_free_;
        free_list_[index].is_free = 1;
        next_free_ = index;
        --num_alloc;
    }

private:
    T *pool_;
    size_t count_;
    uint32_t next_free_;
    uint32_t num_alloc_;
    struct FreeList *free_list_;
};
