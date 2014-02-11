/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_CE_PTHREAD

#include "debug.h"

#include "ocr-policy-domain.h"

#include "ocr-errors.h"
#include "ocr-sysboot.h"
#include "ocr-hal.h"
#include "utils/ocr-utils.h"

#include "ce-pthread-comm-platform.h"

#define DEBUG_TYPE COMM_PLATFORM

void cePthreadCommDestruct (ocrCommPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void cePthreadCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    u32 numXE;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*) commPlatform;
    commPlatform->pd = PD;
    numXE = PD->neighborCount;
    numXE = 8;  // FIXME: Temporary hardcoding
    commPlatformCePthread->numXE = numXE;
    commPlatformCePthread->xeMessage = (volatile bool **)runtimeChunkAlloc(numXE * sizeof(volatile bool *), NULL);
    commPlatformCePthread->ceMessage = (volatile bool **)runtimeChunkAlloc(numXE * sizeof(volatile bool *), NULL);
    commPlatformCePthread->xeMessagePtr = (volatile ocrPolicyMsg_t ***)runtimeChunkAlloc(numXE * sizeof(volatile ocrPolicyMsg_t **), NULL);
    commPlatformCePthread->ceMessagePtr = (volatile ocrPolicyMsg_t ***)runtimeChunkAlloc(numXE * sizeof(volatile ocrPolicyMsg_t **), NULL);
    commPlatformCePthread->ceMessageBuffer = (ocrPolicyMsg_t **)runtimeChunkAlloc(numXE * sizeof(ocrPolicyMsg_t *), NULL);
    commPlatformCePthread->xeActiveFlag = (bool *)runtimeChunkAlloc(numXE * sizeof(bool), false);
    return;
}

void cePthreadCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    u32 i;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*) commPlatform;
    for (i = 0; i < commPlatformCePthread->numXE; i++) {
        ASSERT(commPlatformCePthread->xeMessage[i]);
        ASSERT(commPlatformCePthread->ceMessage[i]);
        ASSERT(commPlatformCePthread->xeMessagePtr[i]);
        ASSERT(commPlatformCePthread->ceMessagePtr[i]);
        ASSERT(commPlatformCePthread->ceMessageBuffer[i]);
        ASSERT(commPlatformCePthread->xeActiveFlag[i] == false);
    }
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
    DPRINTF(DEBUG_LVL_INFO, "[CE]: Sending Message: %u Type: %u To: %lu\n", 
            (*message)->type, ((*message)->type & PD_MSG_TYPE_ONLY), target);
    ASSERT(*(commPlatformCePthread->ceMessage[target]) == false); 
    ASSERT(*(commPlatformCePthread->ceMessagePtr[target]) == NULL); 
    if ((*message)->type & PD_MSG_RESPONSE) {
        ASSERT(!((*message)->type & PD_MSG_REQUEST));
        *(commPlatformCePthread->ceMessagePtr[target]) = *message;
        hal_fence();
        *(commPlatformCePthread->ceMessage[target]) = true;
        hal_fence();
        while(*(commPlatformCePthread->ceMessage[target]) == true); // block until XE receives response
    } else {
        ASSERT((*message)->type & PD_MSG_REQUEST);
        memcpy(commPlatformCePthread->ceMessageBuffer[target], (*message), sizeof(ocrPolicyMsg_t));
        *(commPlatformCePthread->ceMessagePtr[target]) = commPlatformCePthread->ceMessageBuffer[target];
        hal_fence();
        *(commPlatformCePthread->ceMessage[target]) = true;
    }
    commPlatformCePthread->xeActiveFlag[target] = false;
    return 0;
}

static bool ceReceivedMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message, u32 mask, u32 xeId, bool * doPrint) {
    bool ret = false;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)self;
    if (commPlatformCePthread->xeActiveFlag[xeId] == true) {
        ASSERT(*(commPlatformCePthread->xeMessage[xeId]) == false);
        ASSERT(*(commPlatformCePthread->xeMessagePtr[xeId]));
        *message = (ocrPolicyMsg_t *)(*(commPlatformCePthread->xeMessagePtr[xeId]));
        ret = true;
    } else if (*(commPlatformCePthread->xeMessage[xeId]) == true) {
        ASSERT(*(commPlatformCePthread->xeMessagePtr[xeId]));
        if ((*(commPlatformCePthread->ceMessage[xeId]) == false) ||                             // CE has no outstanding messages OR
           (((*(commPlatformCePthread->xeMessagePtr[xeId]))->type & PD_MSG_REQ_RESPONSE) == 0)) // request does not require response
        {
            *message = (ocrPolicyMsg_t *)(*(commPlatformCePthread->xeMessagePtr[xeId]));
            *doPrint = true;
            ret = true;
            if(((*(commPlatformCePthread->xeMessagePtr[xeId]))->type & PD_MSG_REQUEST) &&
               ((*(commPlatformCePthread->xeMessagePtr[xeId]))->type & PD_MSG_REQ_RESPONSE))
                commPlatformCePthread->xeActiveFlag[xeId] = true;
        }
        hal_fence();
        *(commPlatformCePthread->xeMessage[xeId]) = false;
    }
    return ret;
}

//NOTE: sanjay: Currently polls for only XE messages.
u8 cePthreadCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message, u32 mask) {
    u32 i, startIdx, numXE;
    bool doPrint = false;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)self;
    startIdx = commPlatformCePthread->startIdx;
    numXE = commPlatformCePthread->numXE;
    for (i = 0; i < numXE; i++) { 
        u32 idx = (startIdx + i) % numXE;
        if (ceReceivedMessage(self, message, mask, idx, &doPrint)) {
            commPlatformCePthread->startIdx = (idx + 1) % numXE;
            if(doPrint) {
                DPRINTF(DEBUG_LVL_INFO, "[CE]: Received Message: %u Type: %u From: %lu\n", 
                    (*message)->type, ((*message)->type & PD_MSG_TYPE_ONLY), (u64)((*message)->srcLocation));
            }
            return 0;
        }
    } 
    return OCR_EAGAIN;
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
    commPlatformCePthread->xeMessage = NULL;
    commPlatformCePthread->ceMessage = NULL;
    commPlatformCePthread->xeMessagePtr = NULL;
    commPlatformCePthread->ceMessagePtr = NULL;
    commPlatformCePthread->ceMessageBuffer = NULL;
    commPlatformCePthread->xeActiveFlag = NULL;
    
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

