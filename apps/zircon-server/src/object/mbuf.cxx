#include "object/mbuf.h"
#include "addrspace.h"
#include "utils/page_alloc.h"

namespace MBufCxx {}

zx_status_t MBuf::write(uint8_t *src, size_t len, bool datagram)
{
    using namespace MBufCxx;

    if (len + size_ > kSizeMax) {
        if (datagram) {
            return ZX_ERR_SHOULD_WAIT;
        }
        /* Otherwise do a short write */
        len = kSizeMax - size_;
    }

    /* Figure out how many page bufs we need to allocate */
    size_t rem = (tail_ != NULL) ? tail_->rem() : 0;
    uint32_t num_buf = 0;
    if (len > rem) {
        num_buf += (len - rem + PageBuf::kPayloadSize - 1) / PageBuf::kPayloadSize;
    }

    /* Check we have enough page bufs available */
    if (get_num_page_avail() < num_buf) {
        return ZX_ERR_NO_MEMORY;
    }

    size_t bytes_written = len;

    /* Write to tail */
    if (rem > 0) {
        size_t nbytes = (len > rem) ? rem : len;
        size_t offset = tail_->off_ + tail_->len_;
        memcpy(&tail_->data_[offset], src, nbytes);
        src += nbytes;
        tail_->len_ += nbytes;
        len -= nbytes;
    }

    /* Alloc page bufs and write to them */
    for (uint32_t i = 0; i < num_buf; ++i) {
        /* Alloc the page */
        PageBuf *pb = new (page_alloc()) PageBuf();

        dprintf(SPEW, "Allocd pagebuf at %p\n", pb);

        /* Write to pb */
        size_t nbytes = (len > PageBuf::kPayloadSize) ? PageBuf::kPayloadSize : len;
        memcpy(&pb->data_[0], src, nbytes);
        src += nbytes;
        pb->len_ += nbytes;
        len -= nbytes;

        /* Append to mbuf */
        append(pb);
    }

    size_ += bytes_written;

    return ZX_OK;
}

zx_status_t MBuf::read(uint8_t *dest, size_t len)
{
    using namespace MBufCxx;

    if (size_ == 0) {
        return ZX_ERR_SHOULD_WAIT;
    }

    /* If len too big, do a short read */
    if (len > size_) {
        len = size_;
    }

    size_t bytes_read = len;

    while (len > 0) {
        /* Copy from head to dest */
        size_t offset = head_->off_;
        size_t nbytes = (len > head_->len_) ? head_->len_ : len;
        memcpy(dest, &head_->data_[offset], nbytes);
        head_->off_ += nbytes;
        head_->len_ -= nbytes;
        len -= nbytes;
        dest += nbytes;

        /* If pagebuf drained, free it */
        if (head_->len_ == 0) {
            PageBuf *pb = pop();
            page_free((void *)pb);
        }
    }

    size_ -= bytes_read;
    return ZX_OK;
}

void MBuf::clear()
{
    using namespace MBufCxx;

    while (head_ != NULL) {
        PageBuf *pb = head_;
        head_ = pb->next_;
        page_free((void *)pb);
    }

    tail_ = NULL;
    size_ = 0;
}

void MBuf::discard(size_t len)
{
    using namespace MBufCxx;

    if (len > size_) {
        len = size_;
    }
    size_t bytes_discarded = len;

    while (len > 0) {
        /* Update off & len of head pagebuf */
        size_t nbytes = (len > head_->len_) ? head_->len_ : len;
        head_->off_ += nbytes;
        head_->len_ -= nbytes;
        len -= nbytes;

        /* If pagebuf drained, free it */
        if (head_->len_ == 0) {
            PageBuf *pb = pop();
            page_free((void *)pb);
        }
    }

    size_ -= bytes_discarded;
}
