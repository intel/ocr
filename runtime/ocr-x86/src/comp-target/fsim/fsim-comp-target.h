/**
 * @brief Compute Platform implemented using pthread
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __COMP_TARGET_FSIM_H__
#define __COMP_TARGET_FSIM_H__

#include "ocr-comp-target.h"
#include "ocr-types.h"
#include "ocr-utils.h"

#include <pthread.h>

typedef struct {
    paramListCompTargetInst_t base;
    void (*routine)(void*);
    void* routineArg;
} paramListCompTargetFSIM_t;

typedef struct {
    ocrCompTarget_t base;

    void (*routine)(void*);
    void* routineArg;
} ocrCompTargetFSIM_t;

typedef struct {
    ocrCompTargetFactory_t base;
} ocrCompTargetFactoryFSIM_t;

ocrCompTargetFactory_t* newCompTargetFactoryFSIM(ocrParamList_t *perType);

#endif /* __COMP_TARGET_FSIM_H__ */

