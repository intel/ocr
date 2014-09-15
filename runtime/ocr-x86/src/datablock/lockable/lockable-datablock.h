/**
 * @brief Simple data-block implementation.
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __DATABLOCK_LOCKABLE_H__
#define __DATABLOCK_LOCKABLE_H__

#include "ocr-config.h"
#ifdef ENABLE_DATABLOCK_LOCKABLE

#include "ocr-allocator.h"
#include "ocr-datablock.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"
#include "ocr-worker.h"

typedef struct {
    ocrDataBlockFactory_t base;
} ocrDataBlockFactoryLockable_t;

typedef union {
    struct {
        volatile u64 flags : 16;
        volatile u64 numUsers : 15;
        volatile u64 internalUsers : 15;
        volatile u64 freeRequested: 1;
        volatile u64 modeLock : 2;
        volatile u64 _padding : 1; //TODO DBX Purpose of this ??
    };
    u64 data;
} ocrDataBlockLockableAttr_t;

// Declared in .c
struct dbWaiter_t;

typedef struct _ocrDataBlockLockable_t {
    ocrDataBlock_t base;

    /* Data for the data-block */
    u32 lock; /**< Lock for this data-block */
    ocrDataBlockLockableAttr_t attributes; /**< Attributes for this data-block */

    ocrGuidTracker_t usersTracker;
    struct _dbWaiter_t * ewWaiterList;  /**< EDTs waiting for exclusive write access */
    struct _dbWaiter_t * itwWaiterList; /**< EDTs waiting for intent-to-write access */
    struct _dbWaiter_t * roWaiterList;  /**< EDTs waiting for read only access */
    ocrLocation_t itwLocation;
    ocrWorker_t * worker; /**< worker currently owning the DB internal lock */
} ocrDataBlockLockable_t;

extern ocrDataBlockFactory_t* newDataBlockFactoryLockable(ocrParamList_t *perType, u32 factoryId);

#endif /* ENABLE_DATABLOCK_LOCKABLE */
#endif /* __DATABLOCK_LOCKABLE_H__ */
