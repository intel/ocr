/**
 * @brief Simple basic x86 implementation of synchronization primitives
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


#include "ocr-types.h"
#include "x86.h"

void createX86(ocrLock_t* self, void* config) {
    ocrLockX86_t *rself = (ocrLockX86_t*)self;
    rself->val = 0;
}

void destructX86(ocrLock_t* self) {
    return;
}

void lockX86(ocrLock_t* self) {
    ocrLockX86_t *rself = (ocrLockX86_t*)self;
    while(!__sync_bool_compare_and_swap(&(rself->val), 0, 1)) ;
}

void unlockX86(ocrLock_t* self) {
    ocrLockX86_t *rself = (ocrLockX86_t*)self;
    asm volatile ("mfence");
    ASSERT(rself->val == 1);
    rself->val = 0;
}

ocrLock_t* newLockX86() {
    ocrLockX86_t *result = (ocrLockX86_t*)malloc(sizeof(ocrLockX86_t));

    result->base.create = &createX86;
    result->base.destruct = &destructX86;
    result->base.lock = &lockX86;
    result->base.unlock = &unlockX86;

    return (ocrLock_t*)result;
}
