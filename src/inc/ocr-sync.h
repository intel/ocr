/**
 * @brief OCR internal interface for locks and other internal
 * synchronization mechanisms
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


#ifndef __OCR_SYNC_H__
#define __OCR_SYNC_H__

#include "ocr-types.h"

/**
 * @brief A model for a very simple lock that
 * can be used in the runtime
 *
 * Locks are not exposed to the user as the EDT
 * model does not require them
 */
typedef struct _ocrLock_t {
    /**
     * @brief Constructor equivalent
     *
     * Constructs a lock
     *
     * @param self          Pointer to this lock
     * @param config        An optional configuration (not currently used)
     */
    void (*create)(struct _ocrLock_t* self, void* config);

    /**
     * @brief Destructor equivalent
     *
     * Cleans up the lock if needed. Does not call free
     *
     * @param self          Pointer to this lock
     */
    void (*destruct)(struct _ocrLock_t* self);

    void (*lock)(struct _ocrLock_t* self);

    void (*unlock)(struct _ocrLock_t* self);
} ocrLock_t;

typedef enum _ocrLockKind {
    OCR_LOCK_DEFAULT = 0,
    OCR_LOCK_X86 = 1
} ocrLockKind;

extern ocrLockKind ocrLockDefaultKind;

/**
 * @brief Allocates a lock
 *
 * The user will need to call "create" on the
 * lock returned to properly initialize it
 *
 * @param type              Type of the lock
 * @return A pointer to the meta-data for the lock
 */
ocrLock_t *newLock(ocrLockKind type);

#endif /* __OCR_SYNC_H__ */
