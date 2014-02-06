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
    u32 i, numXE;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*) commPlatform;
    numXE = PD->neighborCount;
    numXE = 8;  // FIXME: Temporary hardcoding
    commPlatformCePthread->numXE = numXE;

    commPlatformCePthread->requestQueues = (deque_t**) runtimeChunkAlloc(sizeof(deque_t*)*numXE, NULL);
    commPlatformCePthread->responseQueues = (deque_t**) runtimeChunkAlloc(sizeof(deque_t*)*numXE, NULL);
    commPlatformCePthread->requestCounts = (volatile u64 *)runtimeChunkAlloc(PAD_SIZE * numXE * sizeof(u64), NULL);
    commPlatformCePthread->responseCounts = (volatile u64 *)runtimeChunkAlloc(PAD_SIZE * numXE * sizeof(u64), NULL);
    commPlatformCePthread->ceLocalRequestCounts = (u64 *)runtimeChunkAlloc(numXE * sizeof(u64), NULL);

    for (i = 0; i < numXE; i++) {
        commPlatformCePthread->requestQueues[i] = newNonConcurrentQueue(PD, NULL); 
        commPlatformCePthread->responseQueues[i] = newNonConcurrentQueue(PD, NULL); 
        commPlatformCePthread->requestCounts[i * PAD_SIZE] = 0;
        commPlatformCePthread->responseCounts[i * PAD_SIZE] = 0;
        commPlatformCePthread->ceLocalRequestCounts[i] = 0;
    }
    return;
}

void cePthreadCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
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
    DPRINTF(DEBUG_LVL_INFO, "[CE] Sending Message: To %lu Src: %lu Count: %lu Type: %u\n", 
            (u64)target, (u64)((*message)->srcLocation), 
            commPlatformCePthread->responseCounts[target * PAD_SIZE], 
            (*message)->type);
    deque_t * q = commPlatformCePthread->responseQueues[target];
    q->pushAtTail(q, (void*)(*message), 0);
    commPlatformCePthread->responseCounts[target * PAD_SIZE]++;
    return 0;
}

//NOTE: sanjay: Currently polls for only XE messages.
u8 cePthreadCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message, u32 mask) {
    u32 i, startIdx, numXE;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)self;
    startIdx = commPlatformCePthread->startIdx;
    numXE = commPlatformCePthread->numXE;
    for (i = 0; i < numXE; i++) { 
        u32 idx = (startIdx + i) % numXE;
        if (commPlatformCePthread->requestCounts[idx * PAD_SIZE] > commPlatformCePthread->ceLocalRequestCounts[idx]) {
            deque_t * q = commPlatformCePthread->requestQueues[idx];
            *message = (ocrPolicyMsg_t *) q->popFromHead(q, 0); 
            commPlatformCePthread->ceLocalRequestCounts[idx]++;
            DPRINTF(DEBUG_LVL_INFO, "[CE] Received Message: From %lu Idx: %u Count: %lu Type: %u\n", 
                   (u64)((*message)->srcLocation), idx, 
                   commPlatformCePthread->ceLocalRequestCounts[idx], (*message)->type);
            commPlatformCePthread->startIdx = (idx + 1) % numXE;
            return 0;
        }
    } 
    return 1;
}

u8 cePthreadCommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message) {
    while (cePthreadCommPollMessage(self, message, 0) != 0);
    return 0;
}

ocrCommPlatform_t* newCommPlatformCePthread(ocrCommPlatformFactory_t *factory,
                                       ocrParamList_t *perInstance) {

    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformCePthread_t), NULL);

    commPlatformCePthread->base.location = ((paramListCommPlatformInst_t *)perInstance)->location;
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
                                      ocrCommPlatformFactory_t*, ocrParamList_t*),
                                  newCommPlatformCePthread);
    base->destruct = FUNC_ADDR(void (*)(ocrCommPlatformFactory_t*), destructCommPlatformFactoryCePthread);
    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), cePthreadCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t), cePthreadCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t), cePthreadCommStart);
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

