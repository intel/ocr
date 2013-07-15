/**
 * @brief Simple basic x86 implementation of synchronization primitives
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


#ifndef __SYNC_X86_H__
#define __SYNC_X86_H__

#include "ocr-sync.h"
#include "ocr-types.h"
#include "ocr-utils.h"

typedef struct {
    ocrLock_t base;
    u32 val;
} ocrLockX86_t;

typedef struct {
    ocrLockFactory_t base;
} ocrLockFactoryX86_t;

typedef struct {
    ocrAtomic64_t base;
    volatile u64 val;
} ocrAtomic64X86_t;

typedef struct {
    ocrAtomic64Factory_t base;
} ocrAtomic64FactoryX86_t;

typedef struct {
    ocrQueue_t base;
    volatile u64 head, tail, size;
    u64 *content;
    volatile u32 lock;
} ocrQueueX86_t;

typedef struct {
    ocrQueueFactory_t base;
} ocrQueueFactoryX86_t;

ocrLockFactory_t *newLockFactoryX86(ocrParamList_t *typeArg);
ocrAtomic64Factory_t *newAtomic64FactoryX86(ocrParamList_t *typeArg);
ocrQueueFactory_t *newQueueFactoryX86(ocrParamList_t *typeArg);

#endif /* __SYNC_X86_H__ */
