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
#include "policy-domain/ce/ce-policy.h"

#define DEBUG_TYPE COMM_PLATFORM

void cePthreadCommDestruct (ocrCommPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void cePthreadCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t *comm) {
    u32 i, idx, numChannels, xeCount;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*) commPlatform;
    commPlatform->pd = PD;
    commPlatformCePthread->numNeighborChannels = 0;
    xeCount = ((ocrPolicyDomainCe_t*)PD)->xeCount;
    numChannels = xeCount + PD->neighborCount;
    commPlatformCePthread->numChannels = numChannels;
    commPlatformCePthread->numNeighborChannels = PD->neighborCount;
    commPlatformCePthread->channelIdx = 0;
    commPlatformCePthread->channels = (ocrCommChannel_t*)runtimeChunkAlloc(numChannels * sizeof(ocrCommChannel_t), PERSISTENT_CHUNK);
    commPlatformCePthread->neighborChannels = (ocrCommChannel_t**)runtimeChunkAlloc(commPlatformCePthread->numNeighborChannels * sizeof(ocrCommChannel_t*), PERSISTENT_CHUNK);
    for (i = 0; i < numChannels; i++) {
        DPRINTF(DEBUG_LVL_VVERB, "Setting up channel %d: %p \n", i, &(commPlatformCePthread->channels[i]));
        commPlatformCePthread->channels[i].message = NULL;
        commPlatformCePthread->channels[i].remoteCounter = 0;
        commPlatformCePthread->channels[i].localCounter = 0;
        commPlatformCePthread->channels[i].msgCancel = false;
    }

    for (i = 0, idx = xeCount; i < PD->neighborCount; i++) {
        commPlatformCePthread->channels[idx++].remoteLocation = PD->neighbors[i];
    }
    return;
}

void cePthreadCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t *comm) {
    u32 i, j;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*) commPlatform;
    for (i = 0; i < PD->neighborCount; i++) {
        ocrPolicyDomain_t * neighborPD = PD->neighborPDs[i];
        ocrCommPlatformCePthread_t * commPlatformCePthreadNeighbor = (ocrCommPlatformCePthread_t *)neighborPD->commApis[0]->commPlatform;
        ocrCommChannel_t *neighborChannel = NULL;
        for (j = 0; j < commPlatformCePthreadNeighbor->numChannels; j++) {
            if (commPlatformCePthreadNeighbor->channels[j].remoteLocation == PD->myLocation) {
                neighborChannel = &(commPlatformCePthreadNeighbor->channels[j]);
                break;
            }
        }
        ASSERT(neighborChannel != NULL);
        DPRINTF(DEBUG_LVL_VVERB, "Setting up neighbor channel %d pointer %p \n", i, neighborChannel);
        commPlatformCePthread->neighborChannels[i] = neighborChannel;
    }
    return;
}

void cePthreadCommStop(ocrCommPlatform_t * commPlatform) {
    // Nothing to do really
}

void cePthreadCommFinish(ocrCommPlatform_t *commPlatform) {
}

u8 cePthreadCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target, ocrPolicyMsg_t *msg,
                            u64 bufferSize, u64 *id, u32 properties, u32 mask) {
    u32 i;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)self;
    ocrPolicyDomainCe_t *cePD = (ocrPolicyDomainCe_t*)self->pd;
    ASSERT(target != self->pd->myLocation);

    //FIXME: This is currently a hack to know if target is a CE or XE.
    //This needs to be replaced by a location query in future.
    //Bug #135
    ocrCommChannel_t * channel = NULL;
    bool ceTarget = false;
    for (i = 0; i < self->pd->neighborCount; i++) {
        if (target == self->pd->neighbors[i]) {
            ceTarget = true;
            break;
        }
    }

    if (ceTarget) {
        //CE --> CE: Non-blocking sends over a single buffer
        if (msg->type & PD_MSG_REQUEST) {
            ASSERT(!(msg->type & PD_MSG_RESPONSE));
            for (i = 0; i < commPlatformCePthread->numNeighborChannels; i++) {
                if (commPlatformCePthread->neighborChannels[i]->remoteLocation == self->pd->myLocation) {
                    channel = commPlatformCePthread->neighborChannels[i];
                    break;
                }
            }
            ASSERT(channel);
            DPRINTF(DEBUG_LVL_VVERB, "Send CE REQ Type: ix%x Dest: %lu Channel: %p RemoteCounter: %lu LocalCounter: %lu\n",
                    msg->type, target, channel, channel->remoteCounter, channel->localCounter);
            u64 fullMsgSize = 0, marshalledSize = 0;
            ocrPolicyMsgGetMsgSize(msg, &fullMsgSize, &marshalledSize);
            hal_fence();
            ocrPolicyMsg_t * channelMessage = (ocrPolicyMsg_t*)channel->message;
            if (channelMessage == NULL) {
                ocrPolicyMsgMarshallMsg(msg, (u8*)(&(channel->messageBuffer)), MARSHALL_FULL_COPY);
                channel->message = &(channel->messageBuffer);
                hal_fence();
                ++(channel->remoteCounter);
            } else if ((msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_MGT_SHUTDOWN && channelMessage != &(channel->overwriteBuffer)) {
                //Special case for shutdown: Overwrite any exisiting messages in the buffer
                ocrPolicyMsgMarshallMsg(msg, (u8*)(&(channel->overwriteBuffer)), MARSHALL_FULL_COPY);
                if (!__sync_bool_compare_and_swap((&(channel->message)), channelMessage, &(channel->overwriteBuffer))) {
                    ASSERT(channel->message == NULL);
                    channel->message = &(channel->overwriteBuffer);
                    hal_fence();
                    ++(channel->remoteCounter);
                }
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "Send CE Busy Error REQ Type: 0x%x Dest: %lu Channel: %p RemoteCounter: %lu LocalCounter: %lu\n",
                        msg->type, target, channel, channel->remoteCounter, channel->localCounter);
                return OCR_EBUSY;
            }

            if (properties & BLOCKING_SEND_MSG_PROP) {
                do {
                    hal_fence();
                } while(channel->remoteCounter > channel->localCounter);
            }
        } else {
            ASSERT(msg->type & PD_MSG_RESPONSE);
            for (i = cePD->xeCount; i < commPlatformCePthread->numChannels; i++) {
                if (commPlatformCePthread->channels[i].remoteLocation == target) {
                    channel = &(commPlatformCePthread->channels[i]);
                    break;
                }
            }
            ASSERT(channel && channel->message == NULL);
            DPRINTF(DEBUG_LVL_VVERB, "Send CE RESP Type: 0x%x Dest: %lu Channel: %p RemoteCounter: %lu LocalCounter: %lu\n",
                    msg->type, target, channel, channel->remoteCounter, channel->localCounter);
            hal_fence();
            u64 fullMsgSize = 0, marshalledSize = 0;
            ocrPolicyMsgGetMsgSize(msg, &fullMsgSize, &marshalledSize);
            ocrPolicyMsgMarshallMsg(msg, (u8*)(&(channel->messageBuffer)), MARSHALL_FULL_COPY);
            channel->message = &(channel->messageBuffer);
            hal_fence();
            ++(channel->localCounter);
        }

        DPRINTF(DEBUG_LVL_VERB, "Sending message @ %p to %lu of type 0x%x\n",
                msg, (u64)target, msg->type);
    } else {
        //CE --> XE: Blocking sends over a single buffer
        u64 block_size = cePD->xeCount + 1;
        channel = &(commPlatformCePthread->channels[((u64)target) % block_size]);

        ocrPolicyMsg_t * channelMessage = (ocrPolicyMsg_t *)__sync_val_compare_and_swap((&(channel->message)), NULL, msg);
        if (channelMessage != NULL) {
            DPRINTF(DEBUG_LVL_VERB, "Cancelling CE msg @ %p of type 0x%x to %lu; channel msg @ %p of type 0x%x from %lu\n",
                    msg, msg->type, msg->srcLocation, channelMessage, channelMessage->type,
                    channelMessage->srcLocation);
            RESULT_TRUE(__sync_bool_compare_and_swap((&(channel->message)), channelMessage, msg));
            RESULT_TRUE(__sync_bool_compare_and_swap((&(channel->msgCancel)), false, true));
            hal_fence();
            ++(channel->localCounter);
        }
        DPRINTF(DEBUG_LVL_VERB, "Sending message @ %p of type 0x%x to %lu\n",
                msg, msg->type, target);
        hal_fence();
        ++(channel->localCounter);
        do {
            hal_fence();
        } while(channel->localCounter > channel->remoteCounter);
    }

    return 0;
}

u8 cePthreadCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg, u64* bufferSize,
                            u32 properties, u32 *mask) {
    u32 i, startIdx, numChannels;
    u64 localCounter, remoteCounter;
    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)self;

    //Poll for [CE] responses (serve your own self first :-))
    for (i = 0; i < commPlatformCePthread->numNeighborChannels; i++) {
        ocrCommChannel_t * channel = commPlatformCePthread->neighborChannels[i];
        if (channel->localCounter > channel->remoteCounter) {
            ocrPolicyMsg_t * message = (ocrPolicyMsg_t*)channel->message;
            ASSERT(message != NULL);
            RESULT_TRUE(__sync_bool_compare_and_swap((&(channel->message)), message, NULL));
            ocrPolicyMsgUnMarshallMsg((u8*)message, NULL, message, MARSHALL_APPEND);
            hal_fence();
            ++(channel->remoteCounter);
            *msg = message;
            return 0;
        }
    }

    //Poll for [XE,CE] requests
    startIdx = commPlatformCePthread->startIdx;
    numChannels = commPlatformCePthread->numChannels;
    for (i = 0; i < numChannels; i++) {
        u32 idx = (startIdx + i) % numChannels;
        ocrCommChannel_t * channel = &(commPlatformCePthread->channels[idx]);
        localCounter = (u64)channel->localCounter;
        remoteCounter = (u64)channel->remoteCounter;
        if (localCounter < remoteCounter) {
            ocrPolicyMsg_t * message = NULL;
            do {//get a stable message; the original message might get overwritten by shutdown request
                message = (ocrPolicyMsg_t *)channel->message;
                remoteCounter = (u64)channel->remoteCounter;
            } while (!__sync_bool_compare_and_swap((&(channel->message)), message, NULL));
            ASSERT(message != NULL && remoteCounter == (localCounter + 1));
            if (message->type & PD_MSG_REQ_RESPONSE) {
                DPRINTF(DEBUG_LVL_VVERB, "Message from %u needs a response... using buffer @ %p\n",
                        idx, message);
                *msg = message;
            } else {
                DPRINTF(DEBUG_LVL_VVERB, "Message from %u one-way... copying from %p to %p\n",
                        idx, message, &(channel->messageBuffer));
                hal_memCopy(&(channel->messageBuffer), message, sizeof(ocrPolicyMsg_t), false);
                *msg = &(channel->messageBuffer);
            }

            //Unmarshall the CE request
            if (idx >= ((ocrPolicyDomainCe_t*)(self->pd))->xeCount) {
                ocrPolicyMsgUnMarshallMsg((u8*)*msg, NULL, *msg, MARSHALL_APPEND);
            }

            hal_fence();
            ++(channel->localCounter);
            commPlatformCePthread->startIdx = (idx + 1) % numChannels;
            DPRINTF(DEBUG_LVL_VERB, "Received message @ %p of type 0x%x from %lu\n",
                    (*msg), (*msg)->type, (u64)((*msg)->srcLocation));
            return 0;
        }
    }
    return OCR_EAGAIN;
}

u8 cePthreadCommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg, u64* bufferSize,
                            u32 properties, u32 *mask) {
    while (cePthreadCommPollMessage(self, msg, bufferSize, properties, mask) != 0)
        ;
    return 0;
}

u8 cePthreadDestructMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t *msg) {
    // TODO
    return 0;
}

ocrCommPlatform_t* newCommPlatformCePthread(ocrCommPlatformFactory_t *factory,
        ocrParamList_t *perInstance) {

    ocrCommPlatformCePthread_t * commPlatformCePthread = (ocrCommPlatformCePthread_t*)
            runtimeChunkAlloc(sizeof(ocrCommPlatformCePthread_t), PERSISTENT_CHUNK);
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
                                     runtimeChunkAlloc(sizeof(ocrCommPlatformFactoryCePthread_t), NONPERSISTENT_CHUNK);

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
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t **, u64*, u32, u32*),
                                     cePthreadCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t **, u64*, u32, u32*),
                                     cePthreadCommWaitMessage);
    base->platformFcts.destructMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t*),
                                                   cePthreadDestructMessage);

    return base;
}
#endif /* ENABLE_COMM_PLATFORM_CE_PTHREAD */
