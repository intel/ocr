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
#include "utils/ocr-utils.h"

typedef enum _memTargetType_t {
    memTargetShared_id,
    memTargetMax_id
} memTargetType_t;

extern const char * memtarget_types[];

// Shared memory target
#include "mem-target/shared/shared-mem-target.h"

// Add other memory targets using the same pattern as above

ocrMemTargetFactory_t *newMemTargetFactory(memTargetType_t type, ocrParamList_t *typeArg); 

#endif /* __MEM_TARGET_ALL_H__ */
