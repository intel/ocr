/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __COMP_PLATFORM_ALL_H__
#define __COMP_PLATFORM_ALL_H__

#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-utils.h"

typedef enum _compPlatformType_t {
    compPlatformPthread_id,
    compPlatformMax_id,
} compPlatformType_t;

const char * compplatform_types[] = {
    "pthread",
    NULL,
};

// Pthread compute platform
#include "comp-platform/pthread/pthread-comp-platform.h"

// Add other compute platforms using the same pattern as above

inline ocrCompPlatformFactory_t *newCompPlatformFactory(compPlatformType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case compPlatformPthread_id:
        return newCompPlatformFactoryPthread(typeArg);
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __COMP_PLATFORM_ALL_H__ */
