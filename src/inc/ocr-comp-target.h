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

#ifndef __OCR_COMP_TARGET_H__
#define __OCR_COMP_TARGET_H__

#include "ocr-guid.h"
#include "ocr-mappable.h"
#include "ocr-utils.h"

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

typedef struct _paramListCompTargetFact_t {
    ocrParamList_t base;
} paramListCompTargetFact_t;

typedef struct _paramListCompTargetInst_t {
    ocrParamList_t base;
} paramListCompTargetInst_t;

/****************************************************/
/* OCR COMPUTE TARGET                               */
/****************************************************/

typedef struct _launchArg_t {
  void *(*routine)(void*);
  void * arg;
  struct _ocrPolicyDomain_t * PD;
} launchArg_t;

struct _ocrCompTarget_t;

typedef struct _ocrCompTargetFcts_t {
    void (*destruct) (struct _ocrCompTarget_t * self);

    /*! \brief Starts a compute target
     *
     */
    void (*start) (struct _ocrCompTarget_t * self, struct _ocrPolicyDomain_t * PD, launchArg_t * launchArg);

    /*! \brief Stops this comp-target
     */
    void (*stop) (struct _ocrCompTarget_t * self);
} ocrCompTargetFcts_t;

struct _ocrCompPlatform_t;

/** @brief Abstract class to represent OCR compute-target
 *
 * A compute target will run on a compute-platform and emulates a computing
 * resource at the target level.
 *
 */
typedef struct _ocrCompTarget_t {
    ocrMappable_t module;
    ocrGuid_t guid;

    struct _ocrCompPlatform_t ** platforms; /**< Computing platform this compute target
                                    * is executing on */
    u64 platformCount;

    ocrCompTargetFcts_t *fctPtrs;
} ocrCompTarget_t;

/****************************************************/
/* OCR COMPUTE TARGET FACTORY                       */
/****************************************************/
typedef struct _ocrCompTargetFactory_t {
    ocrCompTarget_t * (*instantiate) ( struct _ocrCompTargetFactory_t * factory,
                                       ocrParamList_t *perInstance);
    void (*destruct)(struct _ocrCompTargetFactory_t * factory);

    ocrCompTargetFcts_t targetFcts;
} ocrCompTargetFactory_t;

#endif /* __OCR_COMP_TARGET_H__ */
