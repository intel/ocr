/**
 * @brief Trivial implementation of GUIDs
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-11-13
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
 */

#include "debug.h"
#include "guid/ptr/ptr-guid.h"
#include "ocr-macros.h"

#include <stdlib.h>

typedef struct {
    ocrGuid_t guid;
    ocrGuidKind kind;
} ocrGuidImpl_t;

static void ptrDestruct(ocrGuidProvider_t* self) {
    free(self);
    return;
}

static u8 ptrGetGuid(ocrGuidProvider_t* self, ocrGuid_t* guid, u64 val, ocrGuidKind kind) {
    ocrGuidImpl_t * guidInst = malloc(sizeof(ocrGuidImpl_t));
    guidInst->guid = (ocrGuid_t)val;
    guidInst->kind = kind;
    *guid = (u64) guidInst;
    return 0;
}

static u8 ptrGetVal(ocrGuidProvider_t* self, ocrGuid_t guid, u64* val, ocrGuidKind* kind) {
    ocrGuidImpl_t * guidInst = (ocrGuidImpl_t *) guid;
    *val = (u64) guidInst->guid;
    if(kind)
        *kind = (ocrGuidKind)((u64)(guid) & 0x7);
    return 0;
}

static u8 ptrGetKind(ocrGuidProvider_t* self, ocrGuid_t guid, ocrGuidKind* kind) {
    ocrGuidImpl_t * guidInst = (ocrGuidImpl_t *) guid;
    *kind = guidInst->kind;
    return 0;
}

static u8 ptrReleaseGuid(ocrGuidProvider_t *self, ocrGuid_t guid) {
    free((ocrGuidImpl_t*) guid);
    return 0;
}

static ocrGuidProvider_t* newGuidProviderPtr(ocrGuidProviderFactory_t *factory,
                                             ocrParamList_t *perInstance) {
    ocrGuidProvider_t *base = (ocrGuidProvider_t*)checkedMalloc(
        base, sizeof(ocrGuidProviderPtr_t));
    base->fctPtrs = &(factory->providerFcts);
    return base;
}

/****************************************************/
/* OCR GUID PROVIDER PTR FACTORY                    */
/****************************************************/

static void destructGuidProviderFactoryPtr(ocrGuidProviderFactory_t *factory) {
    free(factory);
}

ocrGuidProviderFactory_t *newGuidProviderFactoryPtr(ocrParamList_t *typeArg) {
    ocrGuidProviderFactory_t *base = (ocrGuidProviderFactory_t*)
        checkedMalloc(base, sizeof(ocrGuidProviderFactoryPtr_t));
    base->instantiate = &newGuidProviderPtr;
    base->destruct = &destructGuidProviderFactoryPtr;
    base->providerFcts.destruct = &ptrDestruct;
    base->providerFcts.getGuid = &ptrGetGuid;
    base->providerFcts.getVal = &ptrGetVal;
    base->providerFcts.getKind = &ptrGetKind;
    base->providerFcts.releaseGuid = &ptrReleaseGuid;

    return base;
}
