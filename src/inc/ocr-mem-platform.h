/**
 * @brief OCR interface to the low level memory interface
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



#ifndef __OCR_MEM_PLATFORM_H__
#define __OCR_MEM_PLATFORM_H__

#include "ocr-types.h"
#include "ocr-mappable.h"
#include "ocr-utils.h"

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/
typedef struct _paramListMemPlatformFact_t {
    ocrParamList_t base;
} paramListMemPlatformFact_t;

typedef struct _paramListMemPlatformInst_t {
    ocrParamList_t base;
} paramListMemPlatformInst_t;


/****************************************************/
/* OCR MEMORY PLATFORM                              */
/****************************************************/

struct _ocrMemPlatform_t;

typedef struct _ocrMemPlatformFcts_t {
    /**
     * @brief Destructor equivalent
     *
     * @param self          Pointer to this low-memory provider
     */
    void (*destruct)(ocrMemPlatform_t* self);

    /**
     * @brief Allocates a chunk of memory for the higher-level
     * allocators to manage
     *
     * @param self          Pointer to this low-memory provider
     * @param size          Size of the chunk to allocate
     * @return Pointer to the chunk of memory allocated
     */
    void* (*allocate)(ocrMemPlatform_t* self, u64 size);

    /**
     * @brief Frees a chunk of memory previously allocated
     * by self using allocate
     *
     * @param self          Pointer to this low-memory provider
     * @param addr          Address to free
     */
    void (*free)(ocrMemPlatform_t* self, void* addr);
} ocrMemPlatformFcts_t;

/**
 * @brief Low-level memory provider.
 *
 * This allows low-level memory allocation (such as malloc)
 *
 * @warning Currently only uses malloc with no consideration for the
 * underlying machine but this will change as support for distributed and/or NUMA
 * architecture comes online. The API may therefore evolve
 */
typedef struct _ocrMemPlatform_t {
    ocrMappable_t module; /**< Base "class" for ocrMemPlatform */

    ocrMemPlatformFcts_t *fctPtrs;
} ocrMemPlatform_t;

/****************************************************/
/* OCR MEMORY PLATFORM FACTORY                      */
/****************************************************/
typedef struct _ocrMemPlatformFactory_t {
    ocrMemPlatform_t * (*instantiate) (struct _ocrMemPlatformFactory_t * factory,
                                       ocrParamList_t* perInstance);
    void (*destruct)(struct _ocrMemPlatformFactory_t * factory);

    ocrMemPlatformFcts_t platformFcts;
} ocrMemPlatformFactory_t;
#endif /* __OCR_MEM_PLATFORM_H__ */
