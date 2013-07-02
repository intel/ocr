/* Copyright (c) 2012, Rice University

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1.  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2.  Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
   3.  Neither the name of Intel Corporation
   nor the names of its contributors may be used to endorse or
   promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef __OCR_COMP_PLATFORM_H__
#define __OCR_COMP_PLATFORM_H__

#include "ocr-mappable.h"
#include "ocr-utils.h"
#include "ocr-guid.h"
#include "ocr-comp-target.h"


/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

typedef struct _paramListCompPlatformFact_t {
    ocrParamList_t base;
} paramListCompPlatformFact_t;

typedef struct _paramListCompPlatformInst_t {
    ocrParamList_t base;
} paramListCompPlatformInst_t;


/****************************************************/
/* OCR COMPUTE PLATFORM                             */
/****************************************************/

struct _ocrCompPlatform_t;

typedef struct _ocrCompPlatformFcts_t {
    void (*destruct)(struct _ocrCompPlatform_t *self);
    /**
     * @brief Starts a thread of execution.
     *
     * The function started will be 'routine' and it will be passed 'routineArg'
     * @todo There was something about a stack size...
     */
    void (*start)(struct _ocrCompPlatform_t *self, struct _ocrPolicyDomain_t * PD, launchArg_t * launchArg);

    /**
     * @brief Stops this tread of execution
     */
    void (*stop)(struct _ocrCompPlatform_t *self);

} ocrCompPlatformFcts_t;

typedef struct _ocrCompPlatform_t {
    ocrMappable_t module;
    ocrCompPlatformFcts_t *fctPtrs;
} ocrCompPlatform_t;


/****************************************************/
/* OCR COMPUTE PLATFORM FACTORY                     */
/****************************************************/

typedef struct _ocrCompPlatformFactory_t {
    ocrCompPlatform_t* (*instantiate)(struct _ocrCompPlatformFactory_t *factory,
                                      ocrParamList_t *perInstance);

    void (*destruct)(struct _ocrCompPlatformFactory_t *factory);

    void (*setIdentifyingFunctions)(struct _ocrCompPlatformFactory_t *factory);

    ocrCompPlatformFcts_t platformFcts;
} ocrCompPlatformFactory_t;

#endif /* __OCR_COMP_PLATFORM_H__ */
