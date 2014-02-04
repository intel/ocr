/**
 * @brief XE communication platform
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */
#ifndef __COMM_PLATFORM_XE_H__
#define __COMM_PLATFORM_XE_H__

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_XE

#include "ocr-comm-platform.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"


typedef struct {
    ocrCommPlatformFactory_t base;
} ocrCommPlatformFactoryXe_t;

typedef struct {
    ocrCommPlatform_t base;
} ocrCommPlatformXe_t;

typedef struct {
    paramListCommPlatformInst_t base;
} paramListCommPlatformXe_t;

extern ocrCommPlatformFactory_t* newCommPlatformFactoryXe(ocrParamList_t *perType);

#endif /* ENABLE_COMM_PLATFORM_XE */
#endif /* __COMM_PLATFORM_XE_H__ */
