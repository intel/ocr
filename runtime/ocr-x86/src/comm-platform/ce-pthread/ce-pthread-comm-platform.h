/**
 * @brief CE_PTHREAD communication platform (no communication; only one policy domain)
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */
#ifndef __COMM_PLATFORM_CE_PTHREAD_H__
#define __COMM_PLATFORM_CE_PTHREAD_H__

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_CE_PTHREAD

#include "ocr-comm-platform.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"
#include "utils/deque.h"


typedef struct {
    ocrCommPlatformFactory_t base;
} ocrCommPlatformFactoryCePthread_t;

typedef struct {
    ocrCommPlatform_t base;
    deque_t ** requestQueues; // Q array (one per XE) for XE's to send a request message to the CE
    deque_t ** responseQueues; // Q array (one per XE) for the CE to send a response message to a XE
    volatile u64 * requestCounts; // padded array of request counters (one per XE)
    volatile u64 * responseCounts; // padded array of response counters (one per XE)
    u64 * ceLocalRequestCounts; // array of counters (one per XE) maintained locally be CE
    u32 numXE;
    u32 startIdx; // start of next poll loop to avoid starvation
} ocrCommPlatformCePthread_t;

typedef struct {
    paramListCommPlatformInst_t base;
} paramListCommPlatformCePthread_t;

extern ocrCommPlatformFactory_t* newCommPlatformFactoryCePthread(ocrParamList_t *perType);

#endif /* ENABLE_COMM_PLATFORM_CE_PTHREAD */
#endif /* __COMM_PLATFORM_CE_PTHREAD_H__ */
