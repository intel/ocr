/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __COMP_TARGET_ALL_H__
#define __COMP_TARGET_ALL_H__

#include "debug.h"
#include "ocr-comp-target.h"
#include "ocr-config.h"
#include "utils/ocr-utils.h"

typedef enum _compTargetType_t {
    compTargetPassThrough_id,
    compTargetMax_id,
} compTargetType_t;

extern const char * comptarget_types[]; 

// Pass-through target (just calls one and only one platform)
#include "comp-target/passthrough/passthrough-comp-target.h"

// Add other compute targets using the same pattern as above

ocrCompTargetFactory_t *newCompTargetFactory(compTargetType_t type, ocrParamList_t *typeArg); 

#endif /* __COMP_TARGET_ALL_H__ */
