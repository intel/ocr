/**
 * @brief OCR compute platforms
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-06-19
 * Copyright (c) 2013, Intel Corporation
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

#include <string.h>
#include "ocr-comp-platform.h"
#include "ocr-guid.h"
#include "ocr-policy-domain.h"
#include "ocr-macros.h"


struct _ocrPolicyDomain_t;
struct _ocrPolicyCtx_t;

/*
struct _ocrPolicyDomain_t bootstrapPD;
struct _ocrPolicyCtx_t bootstrapCtx;

u8 tempGetGuid(struct _ocrPolicyDomain_t * pd, ocrGuid_t *guid, u64 val,
                  ocrGuidKind type, struct _ocrPolicyCtx_t *context) {
    return pd->guidProvider->fctPtrs->getGuid(pd->guidProvider, guid, val, type);
}

struct _ocrPolicyDomain_t *tempPD(void)
{
    static int first_time = 1;

    if (first_time) {
        first_time = 0;
        ocrGuidProviderFactory_t * factory = newGuidProviderFactory(guidPtr_id, NULL);
        ocrGuidProvider_t * guidProvider = factory->instantiate(factory, NULL);
        bootstrapPD.guidProvider = guidProvider;
        bootstrapPD.getGuid = &tempGetGuid;
        tempGetGuid(&bootstrapPD, &(bootstrapPD.guid), (u64)&bootstrapPD, OCR_GUID_POLICY, NULL);
    }
    return &bootstrapPD;
}

struct _ocrPolicyCtx_t* tempGetContext(void) {
    // HUGE HUGE HACK
    static int firstTime = 1;

    if(firstTime) {
        firstTime = 0;

        bootstrapCtx.PD = tempPD();
        bootstrapCtx.sourcePD = bootstrapCtx.PD->guid;
    }

    struct _ocrPolicyCtx_t* result = checkedMalloc(result, sizeof(struct _ocrPolicyCtx_t*));
    result->PD = bootstrapCtx.PD;
    result->sourcePD = bootstrapCtx.sourcePD;
    return result;
}
*/

ocrGuid_t (*getCurrentEDT)() = NULL;
void (*setCurrentEDT)(ocrGuid_t) = NULL;
