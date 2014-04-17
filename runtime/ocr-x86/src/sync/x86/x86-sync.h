/**
 * @brief Simple basic x86 implementation of synchronization primitives
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __SYNC_X86_H__
#define __SYNC_X86_H__

#include "ocr-sync.h"
#include "ocr-types.h"
#include "ocr-utils.h"

typedef struct {
    ocrLock_t base;
    u32 val;
} ocrLockX86_t;

typedef struct {
    ocrLockFactory_t base;
} ocrLockFactoryX86_t;

typedef struct {
    ocrAtomic64_t base;
    volatile u64 val;
} ocrAtomic64X86_t;

typedef struct {
    ocrAtomic64Factory_t base;
} ocrAtomic64FactoryX86_t;

typedef struct {
    ocrQueue_t base;
    volatile u64 head, tail, size;
    u64 *content;
    volatile u32 lock;
} ocrQueueX86_t;

typedef struct {
    ocrQueueFactory_t base;
} ocrQueueFactoryX86_t;

ocrLockFactory_t *newLockFactoryX86(ocrParamList_t *typeArg);
ocrAtomic64Factory_t *newAtomic64FactoryX86(ocrParamList_t *typeArg);
ocrQueueFactory_t *newQueueFactoryX86(ocrParamList_t *typeArg);

#endif /* __SYNC_X86_H__ */
