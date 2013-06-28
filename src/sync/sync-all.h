/**
 * @brief OCR synchronization primitives
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

#ifndef __SYNC_ALL_H__
#define __SYNC_ALL_H__

#include "debug.h"
#include "ocr-sync.h"

typedef enum _syncType_t {
    syncX86_id,
    syncMax_id
} syncType_t;

const char * sync_types[] = {
    "X86",
    NULL
};

#include "sync/x86/x86-sync.h"

inline ocrLockFactory_t *newLockFactory(syncType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case syncX86_id:
        return newLockFactoryX86(typeArg);
    default:
        ASSERT(0);
        return NULL;
    }
}

inline ocrAtomic64Factory_t *newAtomic64Factory(syncType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case syncX86_id:
        return newAtomic64FactoryX86(typeArg);
    default:
        ASSERT(0);
        return NULL;
    }
}

inline ocrQueueFactory_t *newQueueFactory(syncType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case syncX86_id:
        return newQueueFactoryX86(typeArg);
    default:
        ASSERT(0);
        return NULL;
    }
}

#endif /* __SYNC_ALL_H__ */
