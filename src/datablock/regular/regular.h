/**
 * @brief Simple data-block implementation.
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-09-21
 * Copyright (c) 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#ifndef __DATABLOCK_REGULAR_H__
#define __DATABLOCK_REGULAR_H__

#include "ocr-types.h"
#include "ocr-datablock.h"
#include "ocr-allocator.h"

typedef union {
    struct {
        volatile u64 flags : 16;
        volatile numUsers : 15;
        volatile freeRequested: 1;
        volatile _padding : 16;
    };
    u64 data;
} ocrDataBlockRegularAttr_t;

typedef struct _ocrDataBlockRegular_t {
    ocrDataBlock_t base;

    /* Data for the data-block */
    void* ptr; /**< Current address for this data-block */
    ocrAllocator_t* allocator; /**< Current allocator that this data-block belongs to. */
    u32 size; /**< Current size for this data-block */
    volatile u32 lock; /**< Lock for this data-block (used with CAS) */
    ocrDataBlockRegularAttr_t attributes; /**< Attributes for this data-block */

    /**
     * @brief Vector describing the 'slots' available in the following 'users'
     * vector
     *
     * This is a bitvector with the MSB indicating the state of users[0]. A value of 0 for a bit
     * means that the slot is in-use (not-available) and therefore
     * contains a valid user. A value of 1 means that the slot is empty (or that
     * its value is garbage)
     *
     * @todo For now, we only support up to 64 concurrent users for a data-block
     * This restriction will go away in future versions
     */
    u64 usersAvailable;
    ocrGuid_t users[64];
} ocrDataBlockRegular_t;

ocrDataBlock_t* newDataBlockRegular();
#endif /* __DATABLOCK_REGULAR_H__ */
