/**
 * @brief Compute Platform implemented on fsim
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */
#ifndef __COMP_PLATFORM_FSIM_H__
#define __COMP_PLATFORM_FSIM_H__

#include "ocr-config.h"
#ifdef ENABLE_COMP_PLATFORM_FSIM

#include "ocr-comp-platform.h"
#include "ocr-comp-target.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

typedef struct {
    ocrCompPlatformFactory_t base;
    u64 stackSize;
} ocrCompPlatformFactoryFsim_t;

typedef struct {
    ocrCompPlatform_t base;
    u64 stackSize;
    s32 binding;
    bool isMaster;
} ocrCompPlatformFsim_t;

typedef struct {
    paramListCompPlatformInst_t base;
    u64 stackSize;
    s32 binding;
} paramListCompPlatformFsim_t;

extern ocrCompPlatformFactory_t* newCompPlatformFactoryFsim(ocrParamList_t *perType);

#endif /* ENABLE_COMP_PLATFORM_FSIM */
#endif /* __COMP_PLATFORM_FSIM_H__ */
