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

#include "ocr-errors.h"
#include "ocr-hal.h"
#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"

#include "xe-pthread-comm-platform.h"
#include "../ce-pthread/ce-pthread-comm-platform.h"

#define DEBUG_TYPE COMM_PLATFORM

void xePthreadCommDestruct (ocrCommPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void xePthreadCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    commPlatform->pd = PD;
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)commPlatform;
    ocrPolicyDomain_t * cePD = (ocrPolicyDomain_t *)PD->parentLocation;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t *)cePD->workers[0]->computes[0]->platforms[0]->comm;
    commPlatformCePthread->xeMessage[PD->myLocation] = &(commPlatformXePthread->xeMessage);
    commPlatformCePthread->ceMessage[PD->myLocation] = &(commPlatformXePthread->ceMessage);
    commPlatformCePthread->xeMessagePtr[PD->myLocation] = &(commPlatformXePthread->xeMessagePtr);
    commPlatformCePthread->ceMessagePtr[PD->myLocation] = &(commPlatformXePthread->ceMessagePtr);
    commPlatformCePthread->ceMessageBuffer[PD->myLocation] = &(commPlatformXePthread->ceMessageBuffer);
    return;
}

void xePthreadCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    return;
}

void xePthreadCommStop(ocrCommPlatform_t * commPlatform) {
    // Nothing to do really
}

void xePthreadCommFinish(ocrCommPlatform_t *commPlatform) {
}

/* XE Blocking Send */
u8 xePthreadCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target,
                       ocrPolicyMsg_t **message) {
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)self;

    DPRINTF(DEBUG_LVL_INFO, "[XE%lu]: Sending Message: %u Type: %u\n", 
            (u64)commPlatformXePthread->base.pd->myLocation, 
            (*message)->type,
            ((*message)->type & PD_MSG_TYPE_ONLY));

    ASSERT(message != NULL && *message != NULL); 
    ASSERT(commPlatformXePthread->xeMessage == false);
    ASSERT(commPlatformXePthread->xeMessagePtr == NULL);

    if((*message)->type & PD_MSG_REQUEST) {
        ASSERT(!((*message)->type & PD_MSG_RESPONSE));
        if((*message)->type & PD_MSG_REQ_RESPONSE) {
            commPlatformXePthread->xeMessagePtr = *message;
            hal_fence();
            commPlatformXePthread->xeMessage = true;
            hal_fence();
            while(commPlatformXePthread->ceMessage == false); // block until CE responds
        } else {
            u8 idx = 1 - commPlatformXePthread->xeMessageBufferIndex;
            commPlatformXePthread->xeMessageBufferIndex = idx;
            memcpy(&(commPlatformXePthread->xeMessageBuffer[idx]), (*message), sizeof(ocrPolicyMsg_t));
            commPlatformXePthread->xeMessagePtr = &(commPlatformXePthread->xeMessageBuffer[idx]);
            hal_fence();
            commPlatformXePthread->xeMessage = true;
        }
    } else {
        ASSERT((*message)->type & PD_MSG_RESPONSE);
        commPlatformXePthread->xeMessagePtr = *message;
        hal_fence();
        commPlatformXePthread->xeMessage = true;
    }

    hal_fence();
    while(commPlatformXePthread->xeMessage == true); // block until CE has received this msg
    commPlatformXePthread->xeMessagePtr = NULL;

    return 0;
}

u8 xePthreadCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message, u32 mask) {
    ASSERT(0);
}

u8 xePthreadCommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **message) {
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)self;
    ASSERT(commPlatformXePthread->ceMessage && commPlatformXePthread->ceMessagePtr);
    *message = (ocrPolicyMsg_t *)commPlatformXePthread->ceMessagePtr;
    hal_fence();
    commPlatformXePthread->ceMessagePtr = NULL;
    hal_fence();
    commPlatformXePthread->ceMessage = false;

    DPRINTF(DEBUG_LVL_INFO, "[XE%lu]: Received Message: %u Type: %u\n", 
            (u64)commPlatformXePthread->base.pd->myLocation, 
            (*message)->type,
            ((*message)->type & PD_MSG_TYPE_ONLY));

    return 0;
}

ocrCommPlatform_t* newCommPlatformXePthread(ocrCommPlatformFactory_t *factory,
                                       ocrParamList_t *perInstance) {

    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformXePthread_t), NULL);
    ocrCommPlatform_t * derived = (ocrCommPlatform_t *) commPlatformXePthread;
    factory->initialize(factory, derived, perInstance);
    return derived;
}

void initializeCommPlatformXePthread(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * derived, ocrParamList_t * perInstance) {
    initializeCommPlatformOcr(factory, derived, perInstance);
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t *) derived;

    commPlatformXePthread->xeMessage = false;
    commPlatformXePthread->ceMessage = false;
    commPlatformXePthread->xeMessagePtr = NULL;
    commPlatformXePthread->ceMessagePtr = NULL;
    commPlatformXePthread->xeMessageBufferIndex = 0;
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

    
    base->instantiate = &newCommPlatformXePthread;
    base->initialize = &initializeCommPlatformXePthread;
    base->destruct = &destructCommPlatformFactoryXePthread;

    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), xePthreadCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t), xePthreadCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrWorkerType_t), xePthreadCommStart);
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

