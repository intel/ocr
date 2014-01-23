/**
 * @brief XE_PTHREAD communication platform (no communication; only one policy domain)
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */
#ifndef __COMM_PLATFORM_XE_PTHREAD_H__
#define __COMM_PLATFORM_XE_PTHREAD_H__

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_XE_PTHREAD

#include "ocr-comm-platform.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"


typedef struct {
    ocrCommPlatformFactory_t base;
} ocrCommPlatformFactoryXePthread_t;

typedef struct {
    ocrCommPlatform_t base;
    deque_t * requestQueue;
    deque_t * responseQueue;
    volatile int * requestCount; // counter to increment after XE puts in a request
    volatile int * responseCount; // counter that will be incremented by CE when a response arrives
    int xeLocalResponseCount; // counter maintained locally be XE to monitor incomming response
} ocrCommPlatformXePthread_t;

typedef struct {
    paramListCommPlatformInst_t base;
} paramListCommPlatformXePthread_t;

extern ocrCommPlatformFactory_t* newCommPlatformFactoryXePthread(ocrParamList_t *perType);

#endif /* ENABLE_COMM_PLATFORM_XE_PTHREAD */
#endif /* __COMM_PLATFORM_XE_PTHREAD_H__ */
