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

void xePthreadCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t *comm) {
    commPlatform->pd = PD;
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)commPlatform;
    ocrPolicyDomain_t * cePD = (ocrPolicyDomain_t *)PD->parentLocation;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t *)cePD->commApis[0]->commPlatform;
    commPlatformXePthread->channel = &(commPlatformCePthread->channels[PD->myLocation]);
    return;
}

void xePthreadCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t *comm) {
    return;
}

void xePthreadCommStop(ocrCommPlatform_t * commPlatform) {
    // Nothing to do really
}

void xePthreadCommFinish(ocrCommPlatform_t *commPlatform) {
}

/* 
 * XE Non Blocking Send with one buffer space:
 * If buffer is empty send puts msg in it.
 * If buffer is not empty, return "busy" error code.
 */
u8 xePthreadCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target, ocrPolicyMsg_t *msg, 
                            u64 bufferSize, u64 *id, u32 properties, u32 mask) {
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)self;
    ocrCommChannel_t * channel = commPlatformXePthread->channel;
    
    if (!__sync_bool_compare_and_swap ((&(channel->message)), NULL, msg))
        return OCR_EBUSY;
    
    DPRINTF(DEBUG_LVL_INFO, "[XE%lu] sending message @ 0x%lx of type 0x%x\n", 
            (u64)commPlatformXePthread->base.pd->myLocation, msg, msg->type);
    hal_fence();
    ++(channel->xeCounter);
    do {
        hal_fence();
    } while(channel->xeCounter > channel->ceCounter);
    
    if (channel->xeCancel) {
        DPRINTF(DEBUG_LVL_INFO, "[XE%lu] message @ 0x%lx canceled (type 0x%x)\n", 
                (u64)commPlatformXePthread->base.pd->myLocation, msg, msg->type);
        RESULT_TRUE(__sync_bool_compare_and_swap((&(channel->xeCancel)), true, false));
        return OCR_ECANCELED;
    }
    return 0;
}

u8 xePthreadCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg, u32 properties, u32 *mask) {
    ASSERT(0);
}

u8 xePthreadCommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg, u32 properties, u32 *mask) {
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)self;
    ocrCommChannel_t * channel = commPlatformXePthread->channel;
    do {
        hal_fence();
    } while(channel->xeCounter >= channel->ceCounter);
    
    *msg = (ocrPolicyMsg_t *)channel->message;
    ASSERT(*msg);
    
    RESULT_TRUE(__sync_bool_compare_and_swap((&(channel->message)), (*msg), NULL));
    hal_fence();
    ++(channel->xeCounter);
    DPRINTF(DEBUG_LVL_INFO, "[XE%lu] received message @ 0x%lx of type 0x%x\n", 
            (u64)commPlatformXePthread->base.pd->myLocation, (*msg), (*msg)->type);
    return 0;
}

ocrCommPlatform_t* newCommPlatformXePthread(ocrCommPlatformFactory_t *factory,
                                       ocrParamList_t *perInstance) {

    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformXePthread_t), NULL);
    ocrCommPlatform_t * base = (ocrCommPlatform_t *) commPlatformXePthread;
    factory->initialize(factory, base, perInstance);
    return base;
}

void initializeCommPlatformXePthread(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * base, ocrParamList_t * perInstance) {
    initializeCommPlatformOcr(factory, base, perInstance);
    
    ocrCommPlatformXePthread_t * commPlatformXePthread = (ocrCommPlatformXePthread_t *) base;

    commPlatformXePthread->channel = NULL;
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
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*, ocrCommApi_t *),
                                                  xePthreadCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*, ocrCommApi_t *),
                                                  xePthreadCommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), xePthreadCommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), xePthreadCommFinish);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrLocation_t, ocrPolicyMsg_t *, u64, u64*, u32, u32), 
                                               xePthreadCommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t **, u32, u32*),
                                               xePthreadCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t **, u32, u32*),
                                               xePthreadCommWaitMessage);

    return base;
}
#endif /* ENABLE_COMM_PLATFORM_XE_PTHREAD */
