/**
 * @brief OCR memory targets
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

#ifndef __MEM_TARGET_ALL_H__
#define __MEM_TARGET_ALL_H__

#include "debug.h"
#include "ocr-mem-target.h"
#include "ocr-utils.h"

typedef enum _memTargetType_t {
    memTargetShared_id
} memTargetType_t;

// Shared memory target
#include "mem-target/shared/shared-mem-target.h"

// Add other memory targets using the same pattern as above

inline ocrMemTargetFactory_t *newMemTargetFactory(memTargetType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case memTargetShared_id:
        return newMemTargetFactoryShared(typeArg);
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __MEM_TARGET_ALL_H__ */
extern ocrMemPlatform_t * newMemPlatformMalloc(ocrMemPlatformFactory_t * factory, void * perTypeConfig, void * perInstanceConfig);

ocrMemPlatform_t* newMemPlatform(ocrMemPlatformKind type, void * perTypeConfig, void * perInstanceConfig) {
    if(type == OCR_MEMPLATFORM_DEFAULT) type = ocrMemPlatformDefaultKind;
    switch(type) {
    case OCR_MEMPLATFORM_MALLOC:
        return newMemPlatformMalloc(NULL, perTypeConfig, perInstanceConfig);
        break;
    default:
        ASSERT(0);
    }
    return NULL;
}
