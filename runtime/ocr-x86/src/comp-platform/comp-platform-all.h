/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __COMP_PLATFORM_ALL_H__
#define __COMP_PLATFORM_ALL_H__

#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-config.h"
#include "utils/ocr-utils.h"

typedef enum _compPlatformType_t {
    compPlatformPthread_id,
    compPlatformFsim_id,
    compPlatformMax_id,
} compPlatformType_t;

extern const char * compplatform_types[];

// Pthread compute platform
#include "comp-platform/pthread/pthread-comp-platform.h"
#include "comp-platform/fsim/fsim-comp-platform.h"

// Add other compute platforms using the same pattern as above

ocrCompPlatformFactory_t *newCompPlatformFactory(compPlatformType_t type, ocrParamList_t *typeArg);

#endif /* __COMP_PLATFORM_ALL_H__ */
