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

void cePthreadCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t *comm) {
    u32 i, numXE;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*) commPlatform;
    commPlatform->pd = PD;
    numXE = PD->neighborCount;
    commPlatformCePthread->numXE = numXE;
    commPlatformCePthread->channels = (ocrCommChannel_t*)runtimeChunkAlloc(numXE * sizeof(ocrCommChannel_t), NULL);
    for (i = 0; i < numXE; i++) {
        commPlatformCePthread->channels[i].message = NULL;
        commPlatformCePthread->channels[i].xeCounter = 0;
        commPlatformCePthread->channels[i].ceCounter = 0;
    }
    // Set my location to be what is expected by the XEs (ie: the address of my PD)
    PD->myLocation = (u64)PD;
    return;
}

void cePthreadCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t *comm) {
    return;
}

void cePthreadCommStop(ocrCommPlatform_t * commPlatform) {
    // Nothing to do really
}

void cePthreadCommFinish(ocrCommPlatform_t *commPlatform) {
}

u8 cePthreadCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target, ocrPolicyMsg_t *msg,
                            u64 bufferSize, u64 *id, u32 properties, u32 mask) {
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)self;
    ocrCommChannel_t * channel = &(commPlatformCePthread->channels[target]);
    ocrPolicyMsg_t * channelMessage = (ocrPolicyMsg_t *)__sync_val_compare_and_swap((&(channel->message)), NULL, msg);
    if (channelMessage != NULL) {
        DPRINTF(DEBUG_LVL_VERB, "[CE] cancelling CE msg @ %p of type 0x%x to %lu; channel msg @ %p of type 0x%x from %lu\n",
                msg, msg->type, msg->srcLocation, channelMessage, channelMessage->type,
                channelMessage->srcLocation);
        RESULT_TRUE(__sync_bool_compare_and_swap((&(channel->message)), channelMessage, msg));
        RESULT_TRUE(__sync_bool_compare_and_swap((&(channel->xeCancel)), false, true));
        hal_fence();
        ++(channel->ceCounter);
    }
    DPRINTF(DEBUG_LVL_VERB, "[CE] sending message @ %p of type 0x%x to %lu\n",
            msg, msg->type, target);
    hal_fence();
    ++(channel->ceCounter);
    do {
        hal_fence();
    } while(channel->ceCounter > channel->xeCounter);

    return 0;
}

//NOTE: sanjay: Currently polls for only XE messages.
u8 cePthreadCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg, u32 properties, u32 *mask) {
    u32 i, startIdx, numXE;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)self;
    startIdx = commPlatformCePthread->startIdx;
    numXE = commPlatformCePthread->numXE;
    for (i = 0; i < numXE; i++) {
        u32 idx = (startIdx + i) % numXE;
        ocrCommChannel_t * channel = &(commPlatformCePthread->channels[idx]);
        if (channel->ceCounter < channel->xeCounter) {
            ocrPolicyMsg_t * message = (ocrPolicyMsg_t *)channel->message;
            ASSERT(message);
            if (channel->message->type & PD_MSG_REQ_RESPONSE) {
                DPRINTF(DEBUG_LVL_VVERB, "[CE] message from %u needs a response... using buffer @ %p\n",
                        idx, message);
                *msg = message;
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "[CE] message from %u one-way... copying from %p to %p\n",
                        idx, message, &(channel->messageBuffer));
                hal_memCopy(&(channel->messageBuffer), message, sizeof(ocrPolicyMsg_t), false);
                *msg = &(channel->messageBuffer);
            }
            RESULT_TRUE(__sync_bool_compare_and_swap((&(channel->message)), message, NULL));
            hal_fence();
            ++(channel->ceCounter);
            commPlatformCePthread->startIdx = (idx + 1) % numXE;
            DPRINTF(DEBUG_LVL_VERB, "[CE] received message @ %p of type 0x%x from %lu\n",
                    (*msg), (*msg)->type, (u64)((*msg)->srcLocation));
            return 0;
        }
    }
    return OCR_EAGAIN;
}

u8 cePthreadCommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg, u32 properties, u32 *mask) {
    while (cePthreadCommPollMessage(self, msg, properties, mask) != 0)
        ;
    return 0;
}

ocrCommPlatform_t* newCommPlatformCePthread(ocrCommPlatformFactory_t *factory,
        ocrParamList_t *perInstance) {

    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)
            runtimeChunkAlloc(sizeof(ocrCommPlatformCePthread_t), NULL);
    ocrCommPlatform_t * base = (ocrCommPlatform_t *) commPlatformCePthread;
    factory->initialize(factory, base, perInstance);
    return base;
}

void initializeCommPlatformCePthread(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * base, ocrParamList_t * perInstance) {
    initializeCommPlatformOcr(factory, base, perInstance);

    ocrCommPlatformCePthread_t *commPlatformCePthread = (ocrCommPlatformCePthread_t*)base;
    commPlatformCePthread->channels = NULL;
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

    base->instantiate = &newCommPlatformCePthread;
    base->initialize = &initializeCommPlatformCePthread;
    base->destruct = &destructCommPlatformFactoryCePthread;

    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), cePthreadCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*, ocrCommApi_t *),
                                         cePthreadCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*, ocrCommApi_t *),
                                         cePthreadCommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), cePthreadCommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), cePthreadCommFinish);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrLocation_t, ocrPolicyMsg_t *, u64, u64*, u32, u32),
                                     cePthreadCommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t **, u32, u32*),
                                     cePthreadCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t **, u32, u32*),
                                     cePthreadCommWaitMessage);

    return base;
}
#endif /* ENABLE_COMM_PLATFORM_CE_PTHREAD */
