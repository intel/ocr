/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __COMM_PLATFORM_ALL_H__
#define __COMM_PLATFORM_ALL_H__

#include "debug.h"
#include "ocr-comm-platform.h"
#include "ocr-config.h"
#include "utils/ocr-utils.h"

typedef enum _commPlatformType_t {
    commPlatformNull_id,
    commPlatformMax_id,
} commPlatformType_t;

const char * commplatform_types[] = {
    "None",
    NULL,
};

// NULL communication platform
#include "comm-platform/null/null-comm-platform.h"

// Add other communication platforms using the same pattern as above

inline ocrCommPlatformFactory_t *newCommPlatformFactory(commPlatformType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_COMM_PLATFORM_NULL
    case commPlatformNull_id:
        return newCommPlatformFactoryNull(typeArg);
#endif
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __COMM_PLATFORM_ALL_H__ */
