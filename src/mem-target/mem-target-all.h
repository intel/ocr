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
    case memTargetShared_id:
        return newMemTargetFactoryShared(typeArg);
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __MEM_TARGET_ALL_H__ */
/*
extern ocrMemPlatform_t * newMemPlatformMalloc(ocrMemPlatformFactory_t * factory, void * perTypeConfig, void * perInstanceConfig);

ocrMemPlatform_t* newMemPlatform(ocrMemPlatformKind type, void * perTypeConfig, void * perInstanceConfig) {
    if(type == OCR_MEMPLATFORM_DEFAULT) type = ocrMemPlatformDefaultKind;
    switch(type) {
    case OCR_MEMPLATFORM_MALLOC:
        return newMemPlatformMalloc(NULL, perTypeConfig, perInstanceConfig);
        break;
    default:
        ASSERT(0);
    }
    return NULL;
}
*/
