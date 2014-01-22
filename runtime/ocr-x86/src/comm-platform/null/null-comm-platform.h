/**
 * @brief NULL communication platform (no communication; only one policy domain)
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */
#ifndef __COMM_PLATFORM_NULL_H__
#define __COMM_PLATFORM_NULL_H__

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_NULL

#include "ocr-comm-platform.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"


typedef struct {
    ocrCommPlatformFactory_t base;
} ocrCommPlatformFactoryNull_t;

typedef struct {
    ocrCommPlatform_t base;
} ocrCommPlatformNull_t;

typedef struct {
    paramListCommPlatformInst_t base;
} paramListCommPlatformNull_t;

extern ocrCommPlatformFactory_t* newCommPlatformFactoryNull(ocrParamList_t *perType);

#endif /* ENABLE_COMM_PLATFORM_NULL */
#endif /* __COMM_PLATFORM_NULL_H__ */
