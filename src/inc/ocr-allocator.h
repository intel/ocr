/**
 * @brief OCR interface to the memory allocators
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

#ifndef __OCR_ALLOCATOR_H__
#define __OCR_ALLOCATOR_H__

#include "ocr-types.h"
#include "ocr-mem-target.h"
#include "ocr-mappable.h"
#include "ocr-utils.h"


/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

/**
 * @brief Parameter list to create an allocator factory
 */
typedef struct _paramListAllocatorFact_t {
    ocrParamList_t base;
} paramListAllocatorFact_t;

/**
 * @brief Parameter list to create an allocator instance
 */
typedef struct _paramListAllocatorInst_t {
    ocrParamList_t base;
    u64 size;
} paramListAllocatorInst_t;

/****************************************************/
/* OCR ALLOCATOR                                    */
/****************************************************/

struct _ocrAllocator_t;
struct _ocrPolicyDomain_t;

/**
 * @brief Allocator function pointers
 *
 * The function pointers are separate from the allocator instance to allow for
 * the sharing of function pointers for allocators from the same factory
 */
typedef struct _ocrAllocatorFcts_t {
    /**
     * @brief Destructor equivalent
     *
     * Cleans up the allocator. Calls free on self as well as
     * any memory allocated from the low-memory allocators
     *
     * @param self              Pointer to this allocator
     */
    void (*destruct)(struct _ocrAllocator_t* self);


    void (*start)(struct _ocrAllocator_t* self, struct _ocrPolicyDomain_t * PD);

    void (*stop)(struct _ocrAllocator_t* self);

    /**
     * @brief Actual allocation
     *
     * Allocates a continuous chunk of memory of the requested size
     *
     * @param self              Pointer to this allocator
     * @param size              Size to allocate (in bytes)
     * @return NULL if allocation is unsuccessful or the address
     **/
    void* (*allocate)(struct _ocrAllocator_t *self, u64 size);

    /**
     * @brief Frees a previously allocated block
     *
     * The block should have been allocated by the same
      * allocator
     *
     * @param self              Pointer to this allocator
     * @param address           Address to free
     **/
    void (*free)(struct _ocrAllocator_t *self, void* address);

    /**
     * @brief Reallocate within the chunk managed by this allocator
     *
     * @param self              Pointer to this allocator
     * @param address           Address to reallocate
     * @param size              New size
     *
     * Note that regular rules on the special values of
     * address and size apply:
     *   - if address is NULL, equivalent to malloc
     *   - if size is 0, equivalent to free
     */
    void* (*reallocate)(struct _ocrAllocator_t *self, void* address, u64 size);
} ocrAllocatorFcts_t;

struct _ocrMemTarget_t;

/**
 * @brief Allocator is the interface to the allocator to a zone
 * of memory.
 *
 * This is *not* the low-level memory allocator. This allows memory
 * to be managed in "chunks" (for example one per location) and each can
 * have an independent allocator. Specifically, this enables the
 * modeling of scratchpads and makes NUMA memory explicit
 */
typedef struct _ocrAllocator_t {
    ocrMappable_t module;     /**< Base "class" for the allocator */
    ocrGuid_t guid;           /**< The allocator also has a GUID so that
                               * data-blocks can know what allocated/freed them */

    struct _ocrMemTarget_t **memories; /**< Allocators are mapped to ocrMemTarget_t (0+) */
    u64 memoryCount;          /**< Number of memories associated */

    ocrAllocatorFcts_t *fctPtrs;
} ocrAllocator_t;


/****************************************************/
/* OCR ALLOCATOR FACTORY                            */
/****************************************************/

/**
 * @brief Allocator factory
 */
typedef struct _ocrAllocatorFactory_t {
    ocrMappable_t module; /**< Base "class" for the allocator factory */
    /**
     * @brief Allocator factory
     *
     * Initiates a new allocator and returns a pointer
     * to it.
     *
     * @param factory       Pointer to this factory
     * @param size          Total size of the memory to manage with
     *                      this allocator
     * @param
     */
    struct _ocrAllocator_t * (*instantiate)(struct _ocrAllocatorFactory_t * factory,
                                            ocrParamList_t *instanceArg);

    void (*destruct)(struct _ocrAllocatorFactory_t * factory);

    ocrAllocatorFcts_t allocFcts;
} ocrAllocatorFactory_t;

#endif /* __OCR_ALLOCATOR_H__ */
