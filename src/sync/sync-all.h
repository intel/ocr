/**
 * @brief OCR synchronization primitives
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __SYNC_ALL_H__
#define __SYNC_ALL_H__

#include "debug.h"
#include "ocr-sync.h"

typedef enum _syncType_t {
    syncX86_id,
    syncMax_id
} syncType_t;

const char * sync_types[] = {
    "X86",
    NULL
};

#include "sync/x86/x86-sync.h"

inline ocrLockFactory_t *newLockFactory(syncType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case syncX86_id:
        return newLockFactoryX86(typeArg);
    default:
        ASSERT(0);
        return NULL;
    }
}

inline ocrAtomic64Factory_t *newAtomic64Factory(syncType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case syncX86_id:
        return newAtomic64FactoryX86(typeArg);
    default:
        ASSERT(0);
        return NULL;
    }
}

inline ocrQueueFactory_t *newQueueFactory(syncType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case syncX86_id:
        return newQueueFactoryX86(typeArg);
    default:
        ASSERT(0);
        return NULL;
    }
}

#endif /* __SYNC_ALL_H__ */
