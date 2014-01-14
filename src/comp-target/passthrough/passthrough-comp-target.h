/**
 * @brief Compute target that is a pass-through to the underlying platform
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __COMP_TARGET_PASSTHROUGH_H__
#define __COMP_TARGET_PASSTHROUGH_H__

#include "ocr-config.h"
#ifdef ENABLE_COMP_TARGET_PASSTHROUGH

#include "ocr-comp-target.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

typedef struct {
    paramListCompTargetInst_t base;
    void (*routine)(void*);
    void* routineArg;
} paramListCompTargetPt_t;

typedef struct {
    ocrCompTarget_t base;

    void (*routine)(void*);
    void* routineArg;
} ocrCompTargetPt_t;

typedef struct {
    ocrCompTargetFactory_t base;
} ocrCompTargetFactoryPt_t;

ocrCompTargetFactory_t* newCompTargetFactoryPt(ocrParamList_t *perType);

#endif /* ENABLE_COMP_TARGET_PASSTHROUGH */
#endif /* __COMP_TARGET_PASSTHROUGH_H__ */
