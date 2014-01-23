/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_CE_PTHREAD

#include "debug.h"

#include "ocr-policy-domain.h"

#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"

#include "ce-pthread-comm-platform.h"

#define DEBUG_TYPE COMM_PLATFORM

void cePthreadCommDestruct (ocrCommPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void cePthreadCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    // We are a NULL communication so we don't do anything
    u32 i;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*) commPlatform;
    numXE = PD->neighborCount;
    commPlatformCePthread->numXE = numXE;
    commPlatformCePthread->requestQueues = (deque_t**)PD->pdMalloc(PD, numXE * sizeof(deque_t*));
    commPlatformCePthread->responseQueues = (deque_t**)PD->pdMalloc(PD, numXE * sizeof(deque_t*));
    commPlatformCePthread->requestCounts = (volatile int *)PD->pdMalloc(PD, PAD_SIZE * numXE * sizeof(int));
    commPlatformCePthread->responseCounts = (volatile int *)PD->pdMalloc(PD, PAD_SIZE * numXE * sizeof(int));
    commPlatformCePthread->ceLocalRequestCounts = (int *)PD->pdMalloc(PD, numXE * sizeof(int));
    for (i = 0; i < numXE; i++) {
        commPlatformCePthread->requestQueues[i] = newNonConcurrentQueue(PD, NULL); 
        commPlatformCePthread->responseQueues[i] = newNonConcurrentQueue(PD, NULL); 
        commPlatformCePthread->requestCounts[i] = 0;
        commPlatformCePthread->responseCounts[i] = 0;
    	commPlatformCePthread->ceLocalRequestCounts[i] = 0;
    }
    return;
}

void cePthreadCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType,
                   launchArg_t * launchArg) {
    return;
}

void cePthreadCommStop(ocrCommPlatform_t * commPlatform) {
    // Nothing to do really
}

void cePthreadCommFinish(ocrCommPlatform_t *commPlatform) {
}


u8 cePthreadCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target,
                       ocrPolicyMsg_t **message) {
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)self;
    deque_t * q = commPlatformCePthread->responseQueues[target];
    q->pushAtTail(q, message, 0);
    commPlatformCePthread->responseCounters[target]++;
    return 0;
}

//FIXME: sanjay: Currently polls for only XE messages.
u8 cePthreadCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message, u32 mask) {
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)self;
    for (i = 0; i < commPlatformCePthread->numXE; i++) { //FIXME: sanjay: potential starvation loop
        if (commPlatformCePthread->requestCounters[i] > commPlatformCePthread->ceLocalRequestCounts[i]) {
            deque_t * q = commPlatformCePthread->requestQueues[i];
            *message = (ocrPolicyMsg_t *) q->popAtHead(q, 0); 
            return 1;
        }
    } 
    return 0;
}

//FIXME: sanjay: Currently waits for only XE messages.
u8 cePthreadCommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message) {
    while (1) {
        ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)self;
        for (i = 0; i < commPlatformCePthread->numXE; i++) { //FIXME: sanjay: potential starvation loop
            if (commPlatformCePthread->requestCounters[i] > commPlatformCePthread->ceLocalRequestCounts[i]) {
                deque_t * q = commPlatformCePthread->requestQueues[i];
                *message = (ocrPolicyMsg_t *) q->popAtHead(q, 0); 
                return 1;
            }
        } 
    }
    return 0;
}

ocrCommPlatform_t* newCommPlatformCePthread(ocrCommPlatformFactory_t *factory,
                                       ocrLocation_t location, ocrParamList_t *perInstance) {

    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformCePthread_t), NULL);

    commPlatformCePthread->base.location = location;
    commPlatformCePthread->base.fcts = factory->platformFcts;
    commPlatformCePthread->requestQueues = NULL;
    commPlatformCePthread->responseQueues = NULL;
    
    return (ocrCommPlatform_t*)commPlatformCePthread;
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

void destructCommPlatformFactoryCePthread(ocrCommPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCommPlatformFactory_t *newCommPlatformFactoryCePthread(ocrParamList_t *perType) {
    ocrCommPlatformFactory_t *base = (ocrCommPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformFactoryCePthread_t), (void *)1);

    
    base->instantiate = FUNC_ADDR(ocrCommPlatform_t* (*)(
                                      ocrCommPlatformFactory_t*, ocrLocation_t, ocrParamList_t*),
                                  newCommPlatformCePthread);
    base->destruct = FUNC_ADDR(void (*)(ocrCommPlatformFactory_t*), destructCommPlatformFactoryCePthread);
    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), cePthreadCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t), cePthreadCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t, launchArg_t *), cePthreadCommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), cePthreadCommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), cePthreadCommFinish);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrLocation_t,
                                                      ocrPolicyMsg_t **), cePthreadCommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**, u32),
                                               cePthreadCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**),
                                               cePthreadCommWaitMessage);

    return base;
}
#endif /* ENABLE_COMM_PLATFORM_CE_PTHREAD */

