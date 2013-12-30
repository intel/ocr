/**
 * @brief Memory target representing a single giant shared space
 *
 * For our current implementation, this is a pass-through to the x86
 * platform
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __MEM_TARGET_SHARED_H__
#define __MEM_TARGET_SHARED_H__

#include "ocr-config.h"
#ifdef ENABLE_MEM_TARGET_SHARED

#include "ocr-mem-target.h"
#include "ocr-types.h"
#include "ocr-utils.h"

typedef struct {
    ocrMemTargetFactory_t base;
} ocrMemTargetFactoryShared_t;

typedef struct {
    ocrMemTarget_t base;
} ocrMemTargetShared_t;

extern ocrMemTargetFactory_t* newMemTargetFactoryShared(ocrParamList_t *perType);

#endif /* ENABLE_MEM_TARGET_SHARED */
#endif /* __MEM_TARGET_SHARED_H__ */
