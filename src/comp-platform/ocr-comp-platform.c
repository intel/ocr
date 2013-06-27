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

struct _ocrPolicyDomain_t;
struct _ocrPolicyCtx_t;

struct _ocrPolicyDomain_t bootstrapPD;

u8 tempGetGuid(struct _ocrPolicyDomain_t * pd, ocrGuid_t *guid, u64 val,
                  ocrGuidKind type, struct _ocrPolicyCtx_t *context) {
    *guid = (ocrGuid_t)0;
    return 0;
}

struct _ocrPolicyDomain_t *tempPD(void)
{
    static int first_time = 1;

    if (first_time) {
        first_time = 0;
//        memset (&bootstrapPD, 0, sizeof(ocrPolicyDomain_t));
          bootstrapPD.getGuid = &tempGetGuid;
    }
    return &bootstrapPD;
}

extern struct _ocrPolicyDomain_t *tempPD(void);

struct _ocrPolicyDomain_t * (*getCurrentPD)() = &tempPD;

/**
 * This should return a cloned context of the currently executing worker
 */
struct _ocrPolicyCtx_t * (*getCurrentWorkerContext)() = NULL;
ocrGuid_t (*getCurrentEDT)() = NULL;

void (*setCurrentPD)(struct _ocrPolicyDomain_t*) = NULL;
void (*setCurrentWorkerContext)(struct _ocrPolicyCtx_t *) = NULL;
void (*setCurrentEDT)(ocrGuid_t) = NULL;
