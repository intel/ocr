/**
 * @brief OCR memory targets
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __MEM_TARGET_ALL_H__
#define __MEM_TARGET_ALL_H__

#include "debug.h"
#include "ocr-config.h"
#include "ocr-mem-target.h"
#include "ocr-utils.h"

typedef enum _memTargetType_t {
    memTargetShared_id,
    memTargetMax_id
} memTargetType_t;

const char * memtarget_types[] = {
    "shared",
    NULL
};

// Shared memory target
#include "mem-target/shared/shared-mem-target.h"

// Add other memory targets using the same pattern as above

inline ocrMemTargetFactory_t *newMemTargetFactory(memTargetType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_MEM_TARGET_SHARED
    case memTargetShared_id:
        return newMemTargetFactoryShared(typeArg);
#endif
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __MEM_TARGET_ALL_H__ */
