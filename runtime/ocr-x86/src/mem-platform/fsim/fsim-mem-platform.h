/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __MEM_PLATFORM_FSIM_H__
#define __MEM_PLATFORM_FSIM_H__

#include "ocr-config.h"
#ifdef ENABLE_MEM_PLATFORM_FSIM

#include "debug.h"
#include "utils/rangeTracker.h"
#include "ocr-mem-platform.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

typedef struct {
    ocrMemPlatformFactory_t base;
} ocrMemPlatformFactoryFsim_t;

typedef struct {
    ocrMemPlatform_t base;
    rangeTracker_t rangeTracker;
    u32 lock;
} ocrMemPlatformFsim_t;

extern ocrMemPlatformFactory_t* newMemPlatformFactoryFsim(ocrParamList_t *perType);

#endif /* ENABLE_MEM_PLATFORM_FSIM */
#endif /* __MEM_PLATFORM_FSIM_H__ */
