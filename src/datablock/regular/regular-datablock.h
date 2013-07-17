/**
 * @brief Simple data-block implementation.
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __DATABLOCK_REGULAR_H__
#define __DATABLOCK_REGULAR_H__

#include "ocr-allocator.h"
#include "ocr-datablock.h"
#include "ocr-sync.h"
#include "ocr-types.h"
#include "ocr-utils.h"

typedef struct {
    ocrDataBlockFactory_t base;
} ocrDataBlockFactoryRegular_t;

typedef union {
    struct {
        volatile u64 flags : 16;
        volatile u64 numUsers : 15;
        volatile u64 internalUsers : 15;
        volatile u64 freeRequested: 1;
        volatile u64 _padding : 1;
    };
    u64 data;
} ocrDataBlockRegularAttr_t;

typedef struct _ocrDataBlockRegular_t {
    ocrDataBlock_t base;

    /* Data for the data-block */
    ocrLock_t* lock; /**< Lock for this data-block */
    ocrDataBlockRegularAttr_t attributes; /**< Attributes for this data-block */

    ocrGuidTracker_t usersTracker;
} ocrDataBlockRegular_t;

extern ocrDataBlockFactory_t* newDataBlockFactoryRegular(ocrParamList_t *perType);

#endif /* __DATABLOCK_REGULAR_H__ */
