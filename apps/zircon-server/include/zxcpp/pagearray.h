#pragma once

#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include "debug.h"
}

#include "../utils/page_alloc.h"

/* Swap between two & three level page arrays, depending
   on expected size of VMOs. */
#define ZX_PAGEARRAY_USE_THREE_LVL  0

#if ZX_PAGEARRAY_USE_THREE_LVL

/* Three level page table with array indexing */
template <typename T>
class PageArray {
private:
    static constexpr size_t PageSize = 1 << seL4_PageBits;
    static constexpr size_t ItemPerPage = PageSize / sizeof(T);

    struct BotLevel {
        T item_[ItemPerPage];
    };

    static constexpr size_t PtrPerPage = PageSize / sizeof(BotLevel *);

    struct MidLevel {
        BotLevel *bot_[PtrPerPage];
    };

    static_assert(sizeof(BotLevel) == PageSize, "");
    static_assert(sizeof(MidLevel) == PageSize, "");

    static constexpr size_t ItemPerTwoLvl = ItemPerPage * PtrPerPage;

    uint64_t get_top(uint64_t i) { return i / ItemPerTwoLvl; }
    uint64_t get_mid(uint64_t i) { return (i % ItemPerTwoLvl) / ItemPerPage; }
    uint64_t get_bot(uint64_t i) { return i % ItemPerPage; }

public:
    PageArray() = default;
    ~PageArray() {
        /* Assumes lower levels cleared */
        ::free(mid_);
    }

    bool init(size_t size) {
        size_t num_top = (size + ItemPerTwoLvl - 1) / ItemPerTwoLvl;
        mid_ = (MidLevel **)calloc(num_top, sizeof(MidLevel *));
        if (mid_ == NULL) {
            return false;
        }
        size_ = size;
        return true;
    }

    /* Ensure page alloc'd for access at index */
    bool alloc(uint64_t index) {
        uint64_t i = get_top(index);
        if (mid_[i] == NULL) {
            mid_[i] = (MidLevel *)page_alloc_zero();
            if (mid_[i] == NULL) {
                return false;
            }
        }
        uint64_t j = get_mid(index);
        if (mid_[i]->bot_[j] == NULL) {
            mid_[i]->bot_[j] = (BotLevel *)page_alloc_zero();
            if (mid_[i]->bot_[j] == NULL) {
                return false;
            }
        }
        return true;
    }

    T &operator[](uint64_t index) {
        uint64_t i = get_top(index);
        uint64_t j = get_mid(index);
        uint64_t k = get_bot(index);
        return mid_[i]->bot_[j]->item_[k];
    }

    bool has(uint64_t index) {
        uint64_t i = get_top(index);
        uint64_t j = get_mid(index);
        return (mid_[i] != NULL && mid_[i]->bot_[j] != NULL);
    }

    template <typename F>
    void clear(F &&cleanup_func) {
        size_t num_top = (size_ + ItemPerTwoLvl - 1) / ItemPerTwoLvl;
        for (size_t i = 0; i < num_top; ++i) {
            if (mid_[i] != NULL) {
                for (size_t j = 0; j < PtrPerPage; ++j) {
                    if (mid_[i]->bot_[j] != NULL) {
                        for (size_t k = 0; k < ItemPerPage; ++k) {
                            cleanup_func(mid_[i]->bot_[j]->item_[k]);
                        }
                        page_free(mid_[i]->bot_[j]);
                    }
                }
                page_free(mid_[i]);
            }
        }
    }

    /* TODO swap */

private:
    MidLevel **mid_ = NULL;
    size_t size_ = 0;
};

#else

/* Two level page table with array indexing */
template <typename T>
class PageArray {
private:
    static constexpr size_t PageSize = 1 << seL4_PageBits;
    static constexpr size_t ItemPerPage = PageSize / sizeof(T);

    struct BotLevel {
        T item_[ItemPerPage];
    };

    static_assert(sizeof(BotLevel) == PageSize, "");

    uint64_t get_top(uint64_t i) { return i / ItemPerPage; }
    uint64_t get_bot(uint64_t i) { return i % ItemPerPage; }

public:
    PageArray() = default;
    ~PageArray() {
        /* Assumes lower levels cleared */
        ::free(bot_);
    }

    bool init(size_t size) {
        size_t num_top = (size + ItemPerPage - 1) / ItemPerPage;
        bot_ = (BotLevel **)calloc(num_top, sizeof(BotLevel *));
        if (bot_ == NULL) {
            return false;
        }
        size_ = size;
        dprintf(SPEW, "Size of top level: %lu\n", num_top);
        return true;
    }

    /* Ensure page alloc'd for access at index */
    bool alloc(uint64_t index) {
        uint64_t i = get_top(index);
        if (bot_[i] == NULL) {
            bot_[i] = (BotLevel *)page_alloc_zero();
            if (bot_[i] == NULL) {
                return false;
            }
        }
        return true;
    }

    T &operator[](uint64_t index) {
        uint64_t i = get_top(index);
        uint64_t j = get_bot(index);
        return bot_[i]->item_[j];
    }

    bool has(uint64_t index) {
        uint64_t i = get_top(index);
        return (bot_[i] != NULL);
    }

    template <typename F>
    void clear(F &&cleanup_func) {
        size_t num_top = (size_ + ItemPerPage - 1) / ItemPerPage;
        for (size_t i = 0; i < num_top; ++i) {
            if (bot_[i] != NULL) {
                for (size_t j = 0; j < ItemPerPage; ++j) {
                    cleanup_func(bot_[i]->item_[j]);
                }
                page_free(bot_[i]);
            }
        }
    }

    void swap(PageArray &other) {
        /* Swap top level */
        BotLevel **temp_bot = bot_;
        bot_ = other.bot_;
        other.bot_ = temp_bot;
        /* Swap size */
        size_t temp_size = size_;
        size_ = other.size_;
        other.size_ = temp_size;
    }

private:
    BotLevel **bot_ = NULL;
    size_t size_ = 0;
};

#endif /* ZX_PAGEARRAY_USE_THREE_LVL */
