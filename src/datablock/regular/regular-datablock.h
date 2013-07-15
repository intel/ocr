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

#include "ocr-allocator.h"
#include "ocr-datablock.h"
#include "ocr-sync.h"
#include "ocr-types.h"
#include "ocr-utils.h"

typedef struct {
    ocrDataBlockFactory_t base;
} ocrDataBlockFactoryRegular_t;

typedef union {
    struct {
        volatile u64 flags : 16;
        volatile u64 numUsers : 15;
        volatile u64 internalUsers : 15;
        volatile u64 freeRequested: 1;
        volatile u64 _padding : 1;
    };
    u64 data;
} ocrDataBlockRegularAttr_t;

typedef struct _ocrDataBlockRegular_t {
    ocrDataBlock_t base;

    /* Data for the data-block */
    ocrLock_t* lock; /**< Lock for this data-block */
    ocrDataBlockRegularAttr_t attributes; /**< Attributes for this data-block */

    ocrGuidTracker_t usersTracker;
} ocrDataBlockRegular_t;

extern ocrDataBlockFactory_t* newDataBlockFactoryRegular(ocrParamList_t *perType);

#endif /* __DATABLOCK_REGULAR_H__ */
