/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __COMP_TARGET_ALL_H__
#define __COMP_TARGET_ALL_H__

#include "debug.h"
#include "ocr-comp-target.h"
#include "ocr-utils.h"

typedef enum _compTargetType_t {
    compTargetHc_id,
    compTargetMax_id,
} compTargetType_t;

const char * comptarget_types[] = {
    "HC",
    NULL
};

// Pthread compute platform
#include "comp-target/hc/hc-comp-target.h"

// Add other compute targets using the same pattern as above

inline ocrCompTargetFactory_t *newCompTargetFactory(compTargetType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case compTargetHc_id:
        return newCompTargetFactoryHc(typeArg);
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __COMP_TARGET_ALL_H__ */
