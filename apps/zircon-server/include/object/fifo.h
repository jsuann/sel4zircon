#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <zircon/types.h>
}

#include "object.h"

class ZxFifo final : public ZxObject {
friend zx_status_t create_fifo_pair(uint32_t count, uint32_t elemsize,
        ZxFifo *&fifo0, ZxFifo *&fifo1);
public:
    ZxFifo(uint32_t elem_count, uint32_t elem_size) :
            elem_count_{elem_count}, elem_size_{elem_size} {}

    zx_obj_type_t get_object_type() const final { return ZX_OBJ_TYPE_FIFO; }

    void destroy() override;

    ZxFifo *get_peer() const {
        return peer_;
    }

    zx_status_t write(uint8_t *src, size_t len, uint32_t *actual);
    zx_status_t read(uint8_t *dest, size_t len, uint32_t *actual);

private:
    const uint32_t elem_count_;
    const uint32_t elem_size_;

    uint32_t write_index_ = 0;
    uint32_t read_index_ = 0;

    /* Number of elem slots being used */
    uint32_t num_used_ = 0;

    uint8_t *data_ = NULL;

    ZxFifo *peer_ = NULL;
};

zx_status_t create_fifo_pair(uint32_t count, uint32_t elemsize,
        ZxFifo *&fifo0, ZxFifo *&fifo1);
