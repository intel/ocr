/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_NULL

#include "debug.h"

#include "ocr-policy-domain.h"

#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"

#include "null-comm-platform.h"

#define DEBUG_TYPE COMM_PLATFORM

void nullCommDestruct (ocrCommPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void nullCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    // We are a NULL communication so we don't do anything
    return;
}

void nullCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType,
                   launchArg_t * launchArg) {
    return;
}

void nullCommStop(ocrCommPlatform_t * commPlatform) {
    // Nothing to do really
}

void nullCommFinish(ocrCommPlatform_t *commPlatform) {
}


u8 nullCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target,
                       ocrPolicyMsg_t **message) {
    ASSERT(0); // We cannot send any messages
    return 0;
}

u8 nullCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message, u32 mask) {
    ASSERT(0);
    return 0;
}

u8 nullCommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message) {
    ASSERT(0);
    return 0;
}

ocrCommPlatform_t* newCommPlatformNull(ocrCommPlatformFactory_t *factory,
                                       ocrLocation_t location, ocrParamList_t *perInstance) {

    ocrCommPlatformNull_t * commPlatformNull = (ocrCommPlatformNull_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformNull_t), NULL);

    commPlatformNull->base.location = location;
    commPlatformNull->base.fcts = factory->platformFcts;
    
    return (ocrCommPlatform_t*)commPlatformNull;
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

void destructCommPlatformFactoryNull(ocrCommPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCommPlatformFactory_t *newCommPlatformFactoryNull(ocrParamList_t *perType) {
    ocrCommPlatformFactory_t *base = (ocrCommPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformFactoryNull_t), (void *)1);

    
    base->instantiate = FUNC_ADDR(ocrCommPlatform_t* (*)(
                                      ocrCommPlatformFactory_t*, ocrLocation_t, ocrParamList_t*),
                                  newCommPlatformNull);
    base->destruct = FUNC_ADDR(void (*)(ocrCommPlatformFactory_t*), destructCommPlatformFactoryNull);
    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), nullCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t), nullCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t, launchArg_t *), nullCommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), nullCommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), nullCommFinish);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrLocation_t,
                                                      ocrPolicyMsg_t **), nullCommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**, u32),
                                               nullCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**),
                                               nullCommWaitMessage);

    return base;
}
#endif /* ENABLE_COMM_PLATFORM_NULL */

