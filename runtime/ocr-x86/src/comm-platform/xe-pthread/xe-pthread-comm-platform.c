/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_XE_PTHREAD

#include "debug.h"

#include "ocr-policy-domain.h"
#include "ocr-worker.h"
#include "ocr-comp-target.h"
#include "ocr-comp-platform.h"

#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"

#include "xe-pthread-comm-platform.h"
#include "../ce-pthread/ce-pthread-comm-platform.h"

#define DEBUG_TYPE COMM_PLATFORM

void xePthreadCommDestruct (ocrCommPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void xePthreadCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)commPlatform;
    ocrPolicyDomain_t * cePD = (ocrPolicyDomain_t *)PD->parentLocation;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t *)cePD->workers[0]->computes[0]->platforms[0]->comm;
    ASSERT(PD->myLocation < cePD->neighborCount);
    commPlatformXePthread->requestQueue = commPlatformCePthread->requestQueues[PD->myLocation]; 
    commPlatformXePthread->responseQueue = commPlatformCePthread->responseQueues[PD->myLocation];
    commPlatformXePthread->requestCount = commPlatformCePthread->requestCounts + (PD->myLocation * PAD_SIZE);
    commPlatformXePthread->responseCount = commPlatformCePthread->responseCounts + (PD->myLocation * PAD_SIZE);
    commPlatformXePthread->xeLocalResponseCount = 0;
    return;
}

void xePthreadCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType,
                   launchArg_t * launchArg) {
    return;
}

void xePthreadCommStop(ocrCommPlatform_t * commPlatform) {
    // Nothing to do really
}

void xePthreadCommFinish(ocrCommPlatform_t *commPlatform) {
}


u8 xePthreadCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target,
                       ocrPolicyMsg_t **message) {
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)self;
    commPlatformXePthread->requestQueue->pushAtTail(commPlatformXePthread->requestQueue, (void*)message, 0);
    *commPlatformXePthread->requestCount += 1;
    return 0;
}

u8 xePthreadCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message, u32 mask) {
    ASSERT(0);
    return 0;
}

u8 xePthreadCommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message) {
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)self;
    //FIXME: Currently all sends are blocking. This will not work when non-blocking send messages occur.
    //In future, either we need a list(concurrent)  or a local buffer of unclaimed responses (nonconcurrrent).
    while(*(commPlatformXePthread->responseCount) <= commPlatformXePthread->xeLocalResponseCount); 
    *message = (ocrPolicyMsg_t *)commPlatformXePthread->responseQueue->popFromHead(commPlatformXePthread->responseQueue, 0);
    commPlatformXePthread->xeLocalResponseCount++;
    return 0;
}

ocrCommPlatform_t* newCommPlatformXePthread(ocrCommPlatformFactory_t *factory,
                                       ocrParamList_t *perInstance) {

    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformXePthread_t), NULL);

    commPlatformXePthread->base.location = ((paramListCommPlatformInst_t *)perInstance)->location;
    commPlatformXePthread->base.fcts = factory->platformFcts;
    
    return (ocrCommPlatform_t*)commPlatformXePthread;
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

void destructCommPlatformFactoryXePthread(ocrCommPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCommPlatformFactory_t *newCommPlatformFactoryXePthread(ocrParamList_t *perType) {
    ocrCommPlatformFactory_t *base = (ocrCommPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformFactoryXePthread_t), (void *)1);

    
    base->instantiate = FUNC_ADDR(ocrCommPlatform_t* (*)(
                                      ocrCommPlatformFactory_t*, ocrParamList_t*),
                                  newCommPlatformXePthread);
    base->destruct = FUNC_ADDR(void (*)(ocrCommPlatformFactory_t*), destructCommPlatformFactoryXePthread);
    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), xePthreadCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t), xePthreadCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t, launchArg_t *), xePthreadCommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), xePthreadCommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), xePthreadCommFinish);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrLocation_t,
                                                      ocrPolicyMsg_t **), xePthreadCommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**, u32),
                                               xePthreadCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**),
                                               xePthreadCommWaitMessage);

    return base;
}
#endif /* ENABLE_COMM_PLATFORM_XE_PTHREAD */

