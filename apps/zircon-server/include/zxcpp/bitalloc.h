#pragma once

#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Allocate values using a bitfield */

/* Arbitrary limit: not bigger than a page */
constexpr size_t bitAllocMaxBytes = (1 << 12);

class BitAlloc {
public:
    BitAlloc() = default;
    ~BitAlloc() {}

    bool init(size_t num_bytes) {
        if (num_bytes > bitAllocMaxBytes) {
            return false;
        }
        bitmap_ = (uint8_t *)malloc(num_bytes);
        if (bitmap_ == NULL) {
            return false;
        }
        memset(bitmap_, 0, num_bytes);
        num_bytes_ = num_bytes;
        return true;
    }

    void destroy() {
        /* Make sure we don't use BitAlloc's free! */
        ::free(bitmap_);
    }

    bool alloc(uint32_t &index) {
        for (size_t i = 0; i < num_bytes_; ++i) {
            if (bitmap_[i] != 255) {
                uint8_t mask = 1;
                for (int j = 0; j < 8; ++j) {
                    if (~bitmap_[i] & mask) {
                        bitmap_[i] |= mask;
                        index = (i * 8) + j;
                        return true;
                    }
                    mask <<= 1;
                }
                assert(!"BitAlloc should not get here!");
            }
        }
        return false;
    }

    void free(uint32_t index) {
        uint32_t i = index / 8;
        uint32_t j = index % 8;
        assert(bitmap_[i] & (1 << j));
        bitmap_[i] &= ~(1 << j);
    }

    void print() {
        for (size_t i = 0; i < num_bytes_; ++i)
            dprintf(INFO, "0x%x ", bitmap_[i]);
        dprintf(INFO, "\n");
    }

private:
    uint8_t *bitmap_;
    size_t num_bytes_;
};
