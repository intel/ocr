/**
 * @brief CE communication platform
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */
#ifndef __COMM_PLATFORM_CE_H__
#define __COMM_PLATFORM_CE_H__

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_CE

#include "ocr-comm-platform.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

#include "rmd-arch.h"

typedef struct {
    ocrCommPlatformFactory_t base;
} ocrCommPlatformFactoryCe_t;

typedef struct {
    ocrCommPlatform_t base;
    ocrPolicyDomain_t * pdPtr;  // Our PD
    u64 * rq[MAX_NUM_XE];       // Remote stages for this block's XEs
    u64 * lq[MAX_NUM_XE];       // Local stages for this block's XEs
    u64 pollq;                  // Round-robing queue to poll next
} ocrCommPlatformCe_t;

typedef struct {
    paramListCommPlatformInst_t base;
} paramListCommPlatformCe_t;

extern ocrCommPlatformFactory_t* newCommPlatformFactoryCe(ocrParamList_t *perType);

#endif /* ENABLE_COMM_PLATFORM_CE */
#endif /* __COMM_PLATFORM_CE_H__ */
