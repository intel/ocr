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
    /**
     * @brief Constructor equivalent
     *
     * Constructs an allocator to manage the
     * memory from startAddress to startAddress+size
     *
     * @param self              Pointer to this allocator
     * @param startAddress      Starting point for the range
     *                          of memory managed by this
     *                          allocator
     * @param size              Size managed. Note that the
     *                          size will be reduced by whatever
     *                          space is required for the allocator's
     *                          tracking data-structures
     * @param config            An optional configuration (not currently used)
     */
    void (*create)(struct _ocrAllocator_t* self, void* startAddress, u64 size,
                   void* config);

    /**
     * @brief Destructor equivalent
     *
     * Cleans up the allocator. Does not call free.
     *
     * @param self              Pointer to this allocator
     */
    void (*destruct)(struct _ocrAllocator_t* self);

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

} ocrAllocator_t;

typedef enum _ocrAllocatorKind {
    OCR_ALLOCATOR_DEFAULT = 0,
    OCR_ALLOCATOR_TLSF = 1
} ocrAllocatorKind;


extern ocrAllocatorKind ocrAllocatorDefaultKind;

/**
 * @brief Allocates a new allocator of the type specified
 *
 * The user will need to call "create" on the allocator
 * returned to properly initialize it.
 *
 * @param type              Type of the allocator to return
 *                          Defaults to the default allocator if not specified
 * @return A pointer to the meta-data for the allocator
 */
ocrAllocator_t* newAllocator(ocrAllocatorKind type);

#endif /* __OCR_ALLOCATOR_H__ */
