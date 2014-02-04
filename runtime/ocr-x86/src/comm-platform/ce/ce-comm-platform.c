/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_CE

#include "debug.h"

#include "ocr-policy-domain.h"

#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"

#include "ce-comm-platform.h"

#define DEBUG_TYPE COMM_PLATFORM

void ceCommDestruct (ocrCommPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void ceCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    return;
}

void ceCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType,
                   launchArg_t * launchArg) {
    return;
}

void ceCommStop(ocrCommPlatform_t * commPlatform) {
}

void ceCommFinish(ocrCommPlatform_t *commPlatform) {
}


u8 ceCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target,
                       ocrPolicyMsg_t **message) {
    //TODO: DMA the message into XE's spad and wake it up
    return 0;
}

u8 ceCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message, u32 mask) {
    ASSERT(0);
    // TODO: Fill me
    return 0;
}

u8 ceCommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message) {
    ASSERT(0);
    // TODO: Fill me
    return 0;
}

ocrCommPlatform_t* newCommPlatformCe(ocrCommPlatformFactory_t *factory,
                                       ocrParamList_t *perInstance) {

    ocrCommPlatformCe_t * commPlatformCe = (ocrCommPlatformCe_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformCe_t), NULL);

    commPlatformCe->base.location = ((paramListCommPlatformInst_t *)perInstance)->location;
    commPlatformCe->base.fcts = factory->platformFcts;
    
    return (ocrCommPlatform_t*)commPlatformCe;
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

void destructCommPlatformFactoryCe(ocrCommPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCommPlatformFactory_t *newCommPlatformFactoryCe(ocrParamList_t *perType) {
    ocrCommPlatformFactory_t *base = (ocrCommPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformFactoryCe_t), (void *)1);

    
    base->instantiate = FUNC_ADDR(ocrCommPlatform_t* (*)(
                                      ocrCommPlatformFactory_t*, ocrParamList_t*),
                                  newCommPlatformCe);
    base->destruct = FUNC_ADDR(void (*)(ocrCommPlatformFactory_t*), destructCommPlatformFactoryCe);
    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), ceCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t), ceCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t, launchArg_t *), ceCommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), ceCommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), ceCommFinish);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrLocation_t,
                                                      ocrPolicyMsg_t **), ceCommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**, u32),
                                               ceCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**),
                                               ceCommWaitMessage);

    return base;
}
#endif /* ENABLE_COMM_PLATFORM_CE */

