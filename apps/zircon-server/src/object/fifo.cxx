#include "object/fifo.h"

namespace FifoCxx {
    static constexpr size_t kMaxFifoSize = (1 << seL4_PageBits);
}

zx_status_t create_fifo_pair(uint32_t count, uint32_t elemsize,
        ZxFifo *&fifo1, ZxFifo *&fifo2)
{
    using namespace FifoCxx;

    void *data1, *data2;
    fifo1 = fifo2 = NULL;
    data1 = data2 = NULL;

    if (count == 0 || elemsize == 0 || (count * elemsize) > kMaxFifoSize) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    /* count must be power of 2 */
    if (count & (count - 1)) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    /* alloc data bufs for fifos */
    size_t data_size = count * elemsize;
    data1 = malloc(data_size);
    data2 = malloc(data_size);
    if (data1 == NULL || data2 == NULL) {
        goto error_create_fifo;
    }

    /* allocate fifos */
    fifo1 = allocate_object<ZxFifo>(count, elemsize);
    fifo2 = allocate_object<ZxFifo>(count, elemsize);
    if (fifo1 == NULL || fifo2 == NULL) {
        goto error_create_fifo;
    }

    /* set data bufs, peers, signals */
    fifo1->data_ = (uint8_t *)data1;
    fifo2->data_ = (uint8_t *)data2;
    fifo1->peer_ = fifo2;
    fifo2->peer_ = fifo1;
    fifo1->update_state(0, ZX_FIFO_WRITABLE);
    fifo2->update_state(0, ZX_FIFO_WRITABLE);

    return ZX_OK;

error_create_fifo:
    /* free any alloc'd mem */
    free(data1);
    free(data2);
    free_object(fifo1);
    free_object(fifo2);
    fifo1 = NULL;
    fifo2 = NULL;

    return ZX_ERR_NO_MEMORY;
}

void ZxFifo::destroy()
{
    /* Let peer know we are being destroyed */
    ZxFifo *other = peer_;
    if (other != NULL) {
        other->peer_ = NULL;
    }

    /* Free data buf */
    free(data_);
}

zx_status_t ZxFifo::write(uint8_t *src, size_t len, uint32_t *actual)
{
    size_t count = len / elem_size_;
    if (count == 0) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    /* Work out num empty slots */
    size_t avail = elem_count_ - num_used_;
    if (avail == 0) {
        return ZX_ERR_SHOULD_WAIT;
    }

    bool was_empty = (avail == elem_count_);

    if (count > avail) {
        count = avail;
    }

    /* We won't fail from here */
    *actual = count;
    num_used_ += count;
    while (count > 0) {
        uint32_t offset = (write_index_ * elem_size_);
        memcpy(data_ + offset, src, elem_size_);
        write_index_ = (write_index_ + 1) % elem_count_;
        src += offset;
        --count;
    }

    /* If was empty, set readable */
    if (was_empty) {
        update_state(0u, ZX_FIFO_READABLE);
    }

    /* If now full, clear writeable */
    if (num_used_ == elem_count_) {
        /* Write done by peer, update their state */
        peer_->update_state(ZX_FIFO_WRITABLE, 0u);
    }

    return ZX_OK;
}

zx_status_t ZxFifo::read(uint8_t *dest, size_t len, uint32_t *actual)
{
    size_t count = len / elem_size_;
    if (count == 0) {
        return ZX_ERR_OUT_OF_RANGE;
    }

    /* Work out num empty slots */
    size_t avail = num_used_;
    if (avail == 0) {
        return ZX_ERR_SHOULD_WAIT;
    }

    bool was_full = (avail == elem_count_);

    if (count > avail) {
        count = avail;
    }

    /* We won't fail from here */
    *actual = count;
    num_used_ -= count;
    while (count > 0) {
        uint32_t offset = (read_index_ * elem_size_);
        memcpy(dest, data_ + offset, elem_size_);
        read_index_ = (read_index_ + 1) % elem_count_;
        dest += offset;
        --count;
    }

    /* If was full, set peer writable */
    if (was_full && peer_ != NULL) {
        update_state(0u, ZX_FIFO_READABLE);
    }

    /* If now empty, clear readable */
    if (num_used_ == 0) {
        update_state(ZX_FIFO_READABLE, 0u);
    }

    return ZX_OK;
}
