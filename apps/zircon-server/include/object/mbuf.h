#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

/* Variable size message buffer */
class MBuf {
public:
    MBuf() = default;

    zx_status_t write(uint8_t *src, size_t len, bool datagram);
    zx_status_t read(uint8_t *dest, size_t len);

    void discard(size_t len);
    void clear();

    size_t get_size() const {
        return size_;
    }

    bool is_full() const {
        return size_ >= kSizeMax;
    }

    bool is_empty() const {
        return size_ == 0;
    }

private:
    /* Page of memory that backs a portion of the mbuf */
    struct PageBuf {
        static constexpr size_t kHeaderSize = 8 + (2 * 4);
        static constexpr size_t kPageSize = (1 << seL4_PageBits);
        static constexpr size_t kPayloadSize = kPageSize - kHeaderSize;

        PageBuf *next_ = NULL;
        uint32_t off_ = 0u;
        uint32_t len_ = 0u;
        uint8_t data_[kPayloadSize] = {0};

        size_t rem() const {
            return kPayloadSize - (off_ + len_);
        }
    };
    static_assert(sizeof(PageBuf) == PageBuf::kPageSize, "");

    /* Max size of an mbuf, i.e. max num pages allocated! */
    static constexpr size_t kSizeMax = 2048 * PageBuf::kPayloadSize;

    void append(PageBuf *pb) {
        if (head_ == NULL) {
            head_ = pb;
        } else {
            tail_->next_ = pb;
        }
        tail_ = pb;
    }

    PageBuf *pop() {
        PageBuf *pb = head_;
        head_ = pb->next_;
        if (head_ == NULL) {
            tail_ = NULL;
        }
        return pb;
    }

    /* Read from head */
    PageBuf *head_ = NULL;
    /* Write to tail */
    PageBuf *tail_ = NULL;
    /* Amount written to mbuf */
    size_t size_ = 0u;
};
