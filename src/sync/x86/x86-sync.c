/**
 * @brief Simple basic x86 implementation of synchronization primitives
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "ocr-macros.h"
#include "ocr-types.h"
#include "sync/x86/x86-sync.h"

#include <stdlib.h>

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

static u8 tryLockX86(ocrLock_t* self) {
    ocrLockX86_t *rself = (ocrLockX86_t*)self;
    if(__sync_bool_compare_and_swap(&(rself->val), 0, 1))
        return 1;
    return 0;
}

/* x86 lock factory */
static ocrLock_t* newLockX86(ocrLockFactory_t *factory, ocrParamList_t* perInstance) {
    ocrLockX86_t *result = (ocrLockX86_t*)checkedMalloc(result, sizeof(ocrLockX86_t));

    result->base.fctPtrs = &(factory->lockFcts);
    result->val = 0;

    return (ocrLock_t*)result;
}

static void destructLockFactoryX86(ocrLockFactory_t *self) {
    free(self);
}

ocrLockFactory_t* newLockFactoryX86(ocrParamList_t* perType) {
    ocrLockFactory_t *result = (ocrLockFactory_t*)checkedMalloc(
        result, sizeof(ocrLockFactoryX86_t));
    result->destruct = &destructLockFactoryX86;
    result->instantiate = &newLockX86;
    result->lockFcts.destruct = &destructLockX86;
    result->lockFcts.lock = &lockX86;
    result->lockFcts.unlock = &unlockX86;
    result->lockFcts.trylock = &tryLockX86;

    return result;
}

/* x86 atomic */
static void destructAtomic64X86(ocrAtomic64_t *self) {
    free(self);
}

static u64 xadd64X86(ocrAtomic64_t *self, u64 addValue) {
    ocrAtomic64X86_t *rself = (ocrAtomic64X86_t*)(self);
    return __sync_add_and_fetch(&(rself->val), addValue);
}

static u64 cmpswap64X86(ocrAtomic64_t *self, u64 cmpValue, u64 newValue) {
    ocrAtomic64X86_t *rself = (ocrAtomic64X86_t*)(self);
    return __sync_val_compare_and_swap(&(rself->val), cmpValue, newValue);
}

static u64 val64X86(ocrAtomic64_t *self) {
    ocrAtomic64X86_t *rself = (ocrAtomic64X86_t*)(self);
    return rself->val;
}

/* x86 atomic factory */
static ocrAtomic64_t* newAtomic64X86(ocrAtomic64Factory_t *factory, ocrParamList_t* perInstance) {
    ocrAtomic64X86_t *result = (ocrAtomic64X86_t*)checkedMalloc(
        result, sizeof(ocrAtomic64X86_t));

    result->base.fctPtrs = &(factory->atomicFcts);
    result->val = 0ULL;

    return (ocrAtomic64_t*)result;
}

static void destructAtomic64FactoryX86(ocrAtomic64Factory_t *self) {
    free(self);
}

ocrAtomic64Factory_t* newAtomic64FactoryX86(ocrParamList_t *perType) {
    ocrAtomic64Factory_t *result = (ocrAtomic64Factory_t*)checkedMalloc(
        result, sizeof(ocrAtomic64FactoryX86_t));
    result->destruct = &destructAtomic64FactoryX86;
    result->instantiate = &newAtomic64X86;
    result->atomicFcts.destruct = &destructAtomic64X86;
    result->atomicFcts.xadd = &xadd64X86;
    result->atomicFcts.cmpswap = &cmpswap64X86;
    result->atomicFcts.val = &val64X86;
    return (ocrAtomic64Factory_t*)result;
}

/* x86 queue */

static void destructQueueX86(ocrQueue_t *self) {
    ocrQueueX86_t *rself = (ocrQueueX86_t*)self;
    free(rself->content);
    free(rself);
}

static u64 popHeadX86(ocrQueue_t *self) {
    ocrQueueX86_t *rself = (ocrQueueX86_t*)self;
    u64 head = rself->head, tail = rself->tail;
    u64 val = 0ULL;
    if((tail - head) > 0) {
        val = rself->content[head % rself->size];
        rself->head = rself->head + 1;
    }
    return val;
}

static u64 popTailX86(ocrQueue_t *self) {
    return 0;
}

static u64 pushHeadX86(ocrQueue_t *self, u64 val) {
    return 0;
}

static u64 pushTailX86(ocrQueue_t *self, u64 val) {
    ocrQueueX86_t *rself = (ocrQueueX86_t*)self;
    u8 success = 0;
    s32 size;
    while(!success) {
        size = rself->tail - rself->head;
        if(size == rself->size) {
            ASSERT(0); // Deque full
            return 0;
        }
        if(__sync_bool_compare_and_swap(&(rself->lock), 0, 1)) {
            success = 1;
            rself->content[rself->tail % rself->size] = val;
            rself->tail = rself->tail + 1;
            asm volatile("mfence");
            rself->lock = 0;
        }
    }
    return size + 1;
}

/* x86 queue factory */
static ocrQueue_t* newQueueX86(ocrQueueFactory_t *factory, ocrParamList_t *perInstance) {
    ocrQueueX86_t *result = (ocrQueueX86_t*)checkedMalloc(result, sizeof(ocrQueueX86_t));
//    u64 reqSize = (u64)config;
    u64 reqSize = 32; // TODO: Add config if needed and if we want to keep this!!
//    if(!reqSize) reqSize = 32; // Hard coded for now, make a constant somewhere


    result->base.fctPtrs = &(factory->queueFcts);
    result->head = result->tail = 0ULL;
    result->size = reqSize;
    result->content = (u64*)checkedMalloc(result->content, sizeof(u64)*reqSize);
    result->lock = 0;

    return (ocrQueue_t*)result;
}

static void destructQueueFactoryX86(ocrQueueFactory_t *self) {
    free(self);
}

ocrQueueFactory_t* newQueueFactoryX86(ocrParamList_t *perType) {
    ocrQueueFactory_t *result = (ocrQueueFactory_t*)checkedMalloc(
        result, sizeof(ocrQueueFactoryX86_t));
    result->destruct = &destructQueueFactoryX86;
    result->instantiate = &newQueueX86;

    result->queueFcts.destruct = &destructQueueX86;
    result->queueFcts.popHead = &popHeadX86;
    result->queueFcts.popTail = &popTailX86;
    result->queueFcts.pushHead = &pushHeadX86;
    result->queueFcts.pushTail = &pushTailX86;

    return (ocrQueueFactory_t*)result;
}
