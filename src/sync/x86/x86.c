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

#include <stdlib.h>

#include "ocr-macros.h"
#include "ocr-types.h"
#include "x86.h"
#include "debug.h"

/* x86 lock */
static void destructLockX86(ocrLock_t* self) {
    free(self);
}

static void lockX86(ocrLock_t* self) {
    ocrLockX86_t *rself = (ocrLockX86_t*)self;
    while(!__sync_bool_compare_and_swap(&(rself->val), 0, 1)) ;
}

static void unlockX86(ocrLock_t* self) {
    ocrLockX86_t *rself = (ocrLockX86_t*)self;
    asm volatile ("mfence");
    ASSERT(rself->val == 1);
    rself->val = 0;
}

/* x86 lock factory */
static ocrLock_t* newLockX86(ocrLockFactory_t *self, void* config) {
    ocrLockX86_t *result = (ocrLockX86_t*)malloc(sizeof(ocrLockX86_t));

    result->base.destruct = &destructLockX86;
    result->base.lock = &lockX86;
    result->base.unlock = &unlockX86;

    result->val = 0;

    return (ocrLock_t*)result;
}

static void destructLockFactoryX86(ocrLockFactory_t *self) {
    free(self);
    return;
}

ocrLockFactory_t* newLockFactoryX86(void* config) {
    ocrLockFactoryX86_t *result = (ocrLockFactoryX86_t*)malloc(sizeof(ocrLockFactoryX86_t*));
    result->base.destruct = &destructLockFactoryX86;
    result->base.instantiate = &newLockX86;
    return (ocrLockFactory_t*)result;
}

/* x86 atomic */
static void destructAtomic64X86(ocrAtomic64_t *self) {
    free(self);
    return;
}

static u64 xadd64X86(ocrAtomic64_t *self, u64 addValue) {
    ocrAtomic64X86_t *rself = (ocrAtomic64X86_t*)(self);
    return __sync_add_and_fetch(&(rself->val), addValue)
}

static u64 cmpswap64X86(ocrAtomic64_t *self, u64 cmpValue, u64 newValue) {
    ocrAtomic64X86_t *rself = (ocrAtomic64X86_t*)(self);
    return __sync_val_compare_and_swap(&(rself->val), cmpValue, newVal);
}

/* x86 atomic factory */
static ocrAtomic64_t* newAtomic64X86(ocrAtomicFactory64_t *self, void* config) {
    ocrAtomic64X86_t *result = (ocrAtomic64X86_t*)malloc(sizeof(ocrAtomic64X86_t));

    result->base.destruct = &destructAtomicX86;
    result->base.xadd = &xadd64X86;
    result->base.cmpswap = &cmpswap64X86;

    result->val = 0ULL;

    return (ocrAtomic64_t*)result;
}

static void destructAtomic64FactoryX86(ocrAtomicFactory_t *self) {
    free(self);
    return;
}

ocrAtomicFactory_t* newAtomic64FactoryX86(void* config) {
    ocrAtomicFactoryX86_t *result = (ocrAtomicFactoryX86_t*)malloc(sizeof(ocrAtomicFactoryX86_t*));
    result->base.destruct = &destructAtomic64FactoryX86;
    result->base.instantiate = &newAtomic64X86;
    return (ocrAtomicFactory_t*)result;
}

/* x86 queue */

void destructQueueX86(ocrQueue_t *self) {
    ocrQueueX86_t *rself = (ocrQueueX86_t*)self;
    free(rself->content);
    free(rself);
}

u64 popHeadX86(ocrQueue_t *self) {
    ocrQueueX86_t *rself = (ocrQueueX86_t*)self;
    u64 head = rself->head, tail = rself->tail;
    u64 val = 0ULL;
    if((tail - head) > 0) {
        val = rself->content[head];
        rself->head = (rself->head + 1) % rself->size;
    }
    return val;
}

u64 popTailX86(ocrQueue_t *self) {
    return 0;
}

u64 pushHeadX86(ocrQueue_t *self, u64 val) {
    return 0;
}

u64 pushTailX86(ocrQueue_t *self, u64 val) {
    ocrQueueX86_t *rself = (ocrQueueX86_t*)self;
    u8 success = 0;
    s32 size;
    while(!success) {
        size = rself->tail - rself->head;
        if(size < 0) size += rself->size;
        if(size == rself->size) {
            ASSERT(0); // Deque full
            return 0;
        }
        if(__sync_bool_compare_and_swap(&(rself->lock), 0, 1)) {
            success = 1;
            rself->content[rself->tail] = val;
            rself->tail = (rself->tail + 1) % rself->size;
            asm volatile("mfence");
            rself->lock = 0;
        }
    }
    return size + 1;
}

/* x86 queue factory */
static ocrQueue_t* newQueueX86(ocrQueueFactory_t *self, void* config) {
    ocrQueueX86_t *result = (ocrQueueX86_t*)malloc(sizeof(ocrQueueX86_t));
    u64 reqSize = (u64)config;
    if(!reqSize) reqSize = 32; // Hard coded for now, make a constant somewhere

    result->base.destruct = &destructQueueX86;
    result->base.popHead = &popHeadX86;
    result->base.popTail = &popTailX86;
    result->base.pushHead = &pushHeadX86;
    result->base.pushTail = &pushTailX86;

    result->head = result->tail = 0ULL;
    result->size = reqSize;
    result->content = (u64*)malloc(sizeof(u64)*reqSize);
    result->lock = 0;

    return (ocrQueue_t*)result;
}

ocrQueueFactory_t* newQueueFactoryX86(void* config) {
    ocrQueueFactoryX86_t *result = (ocrQueueFactoryX86_t*)malloc(sizeof(ocrQueueFactoryX86_t*));
    result->base.destruct = &destructQueueFactoryX86;
    result->base.instantiate = &newQueueX86;
    return (ocrQueueFactory_t*)result;
}
static void destructQueueFactoryX86(ocrAtomicFactory_t *self) {
    free(self);
    return;
}
