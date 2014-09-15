/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_API_DELEGATE

#include "debug.h"

#include "ocr-sysboot.h"
#include "ocr-worker.h"
#include "utils/ocr-utils.h"
#include "ocr-comm-platform.h"
#include "comm-api/delegate/delegate-comm-api.h"

#define DEBUG_TYPE COMM_API

// Controls how many times we try to poll for a reply before notifying the scheduler
#define MAX_ACTIVE_WAIT 100

void delegateCommDestruct (ocrCommApi_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void delegateCommBegin(ocrCommApi_t * self, ocrPolicyDomain_t * pd) {
    //Initialize base
    self->pd = pd;
    self->commPlatform->fcts.begin(self->commPlatform, pd, self);
}

void delegateCommStart(ocrCommApi_t * self, ocrPolicyDomain_t * pd) {
    self->commPlatform->fcts.start(self->commPlatform, pd, self);
}

void delegateCommStop(ocrCommApi_t * self) {
    self->commPlatform->fcts.stop(self->commPlatform);
}

void delegateCommFinish(ocrCommApi_t *self) {
    self->commPlatform->fcts.finish(self->commPlatform);
}

static ocrPolicyMsg_t * allocateNewMessage(ocrCommApi_t * self, u32 size) {
    ocrPolicyDomain_t * pd = self->pd;
    ocrPolicyMsg_t * message = pd->fcts.pdMalloc(pd, size);
    return message;
}

/**
 * @brief Destruct a message handler created by this comm-platform
 */
void destructMsgHandlerDelegate(ocrMsgHandle_t * handler) {
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, NULL);
    pd->fcts.pdFree(pd, handler);
}

/**
 * @brief Create a message handle for this comm-platform
 */
ocrMsgHandle_t * createMsgHandlerDelegate(ocrPolicyDomain_t * pd, ocrPolicyMsg_t * message, u32 properties) {
    ocrMsgHandle_t * handle = (ocrMsgHandle_t *) pd->fcts.pdMalloc(pd, sizeof(delegateMsgHandle_t));
    ASSERT(handle != NULL);
    handle->msg = message;
    handle->response = NULL;
    handle->status = HDL_NORMAL;
    handle->destruct = &destructMsgHandlerDelegate;
    handle->properties = properties;
    return handle;
}

/**
 * @brief delegate a message send operation to the scheduler
 */
u8 delegateCommSendMessage(ocrCommApi_t *self, ocrLocation_t target,
                            ocrPolicyMsg_t *message,
                            ocrMsgHandle_t **handle, u32 properties) {
    ocrPolicyDomain_t * pd = self->pd;
    ASSERT((pd->myLocation == message->srcLocation) && (target == message->destLocation));
    ASSERT(pd->myLocation != target); //DIST-TODO not tested sending a message to self

    if (!(properties & PERSIST_MSG_PROP)) {
        // Need to make a copy of the message now.
        // Recall send is returning as soon as the
        // handle is handed over to the scheduler.
        ocrPolicyMsg_t * msgCpy = allocateNewMessage(self, message->size);
        hal_memCopy(msgCpy, message, message->size, false);
        message = msgCpy;
        // Indicates to entities down the line the message is now persistent
        // The rationale is that one-way calls are always deallocated by the
        // comm-platform and two-way calls are always poll/wait at some point
        // by a caller that must free the message.
        properties |= PERSIST_MSG_PROP;
    }
    ocrMsgHandle_t * handlerDelegate = createMsgHandlerDelegate(pd, message, properties);
    DPRINTF(DEBUG_LVL_VVERB,"Delegate API: end message handle=%p, msg=%p, type=0x%x\n",
            handlerDelegate, message, message->type);
    // Give comm handle to policy-domain
    ocrFatGuid_t fatGuid;
    fatGuid.metaDataPtr = handlerDelegate;
    ocrPolicyMsg_t giveMsg;
    getCurrentEnv(NULL, NULL, NULL, &giveMsg);
#define PD_MSG (&giveMsg)
#define PD_TYPE PD_MSG_COMM_GIVE
    giveMsg.type = PD_MSG_COMM_GIVE | PD_MSG_REQUEST;
    PD_MSG_FIELD(guids) = &fatGuid;
    PD_MSG_FIELD(guidCount) = 1;
    PD_MSG_FIELD(properties) = 0;
    PD_MSG_FIELD(type) = OCR_GUID_COMM;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &giveMsg, false));
#undef PD_MSG
#undef PD_TYPE
    if (handle != NULL) {
        // Set the handle for the caller
        *handle = handlerDelegate;
    }
    return 0;
}

/**
 * @brief poll message is a no-op in this delegate implementation
 * Depending on the calling context, the handle can either be:
 *  - handle == NULL
 *  - *handle == NULL
 *  - *handle != NULL
 */
u8 delegateCommPollMessage(ocrCommApi_t *self, ocrMsgHandle_t **handle) {
    ocrPolicyDomain_t * pd = self->pd;
    // Try to take a message from the scheduler and pass the handle as a hint.
    ocrFatGuid_t fatGuid;
    fatGuid.metaDataPtr = handle;
    ocrPolicyMsg_t takeMsg;
    getCurrentEnv(NULL, NULL, NULL, &takeMsg);
#define PD_MSG (&takeMsg)
#define PD_TYPE PD_MSG_COMM_TAKE
    takeMsg.type = PD_MSG_COMM_TAKE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guids) = &fatGuid;
    PD_MSG_FIELD(guidCount) = 1;
    PD_MSG_FIELD(properties) = 0;
    PD_MSG_FIELD(type) = OCR_GUID_COMM;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &takeMsg, true));
#undef PD_MSG
#undef PD_TYPE
    delegateMsgHandle_t * delHandle = (delegateMsgHandle_t *) fatGuid.metaDataPtr;
    if (delHandle != NULL) {
        if (handle != NULL) {
            #ifdef OCR_ASSERT
            // was polling for a specific handle, check if that's what we got
            //DIST-TODO design: comparing polled expected handle and what we get: Is it the handle that's important here or the message id ?
            if (*handle != NULL)
                ASSERT(*handle == ((ocrMsgHandle_t*)delHandle));
            #endif
            // Set the handle for the caller
            *handle = (ocrMsgHandle_t *) delHandle;
        }
        return POLL_MORE_MESSAGE;
    }
    return POLL_NO_MESSAGE;
}

/**
 * @brief Wait for a reply for the last message that has been sent out.
 * Depending on the calling context, the handle can either be:
 *  - handle == NULL
 *  - *handle == NULL
 *  - *handle != NULL
 */
u8 delegateCommWaitMessage(ocrCommApi_t *self, ocrMsgHandle_t **handle) {
    u8 ret = POLL_NO_MESSAGE;
    //DIST-TODO one-way ack: is there a use case for waiting a one-way to complete ?
    while(ret == POLL_NO_MESSAGE) {
        // Try to poll a little
        u64 i = 0;
        while ((i < MAX_ACTIVE_WAIT) && (ret == POLL_NO_MESSAGE)) {
            ret = self->fcts.pollMessage(self,handle);
            i++;
        }
        if (ret == POLL_NO_MESSAGE) {
            // If nothing shows up, transfer control to the scheduler for monitoring progress
            ocrPolicyDomain_t * pd;
            ocrPolicyMsg_t msg;
            getCurrentEnv(&pd, NULL, NULL, &msg);
        #define PD_MSG (&msg)
        #define PD_TYPE PD_MSG_MGT_MONITOR_PROGRESS
            msg.type = PD_MSG_MGT_MONITOR_PROGRESS | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
            //TODO not sure if the caller should register a progress function or if the
            //scheduler should know what to do for each type of monitor progress
            PD_MSG_FIELD(monitoree) = &handle;
            PD_MSG_FIELD(properties) = (0 | MONITOR_PROGRESS_COMM);
            RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));
        #undef PD_MSG
        #undef PD_TYPE
        }
    }
    return ret;
}

ocrCommApi_t* newCommApiDelegate(ocrCommApiFactory_t *factory,
                                       ocrParamList_t *perInstance) {
    ocrCommApiDelegate_t * commPlatformDelegate = (ocrCommApiDelegate_t*)
        runtimeChunkAlloc(sizeof(ocrCommApiDelegate_t), NULL);
    factory->initialize(factory, (ocrCommApi_t*) commPlatformDelegate, perInstance);
    return (ocrCommApi_t*) commPlatformDelegate;
}

/**
 * @brief Initialize an instance of comm-api delegate
 */
void initializeCommApiDelegate(ocrCommApiFactory_t * factory, ocrCommApi_t* self, ocrParamList_t * perInstance) {
    initializeCommApiOcr(factory, self, perInstance);
}

/******************************************************/
/* OCR COMM API DELEGATE FACTORY                      */
/******************************************************/

void destructCommApiFactoryDelegate(ocrCommApiFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCommApiFactory_t *newCommApiFactoryDelegate(ocrParamList_t *perType) {
    ocrCommApiFactory_t *base = (ocrCommApiFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCommApiFactoryDelegate_t), (void *)1);

    base->instantiate = newCommApiDelegate;
    base->initialize = initializeCommApiDelegate;
    base->destruct = destructCommApiFactoryDelegate;
    base->apiFcts.destruct = FUNC_ADDR(void (*)(ocrCommApi_t*), delegateCommDestruct);
    base->apiFcts.begin = FUNC_ADDR(void (*)(ocrCommApi_t*, ocrPolicyDomain_t*), delegateCommBegin);
    base->apiFcts.start = FUNC_ADDR(void (*)(ocrCommApi_t*, ocrPolicyDomain_t*), delegateCommStart);
    base->apiFcts.stop = FUNC_ADDR(void (*)(ocrCommApi_t*), delegateCommStop);
    base->apiFcts.finish = FUNC_ADDR(void (*)(ocrCommApi_t*), delegateCommFinish);
    base->apiFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommApi_t*, ocrLocation_t,
                                                      ocrPolicyMsg_t *, ocrMsgHandle_t **, u32), delegateCommSendMessage);
    base->apiFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommApi_t*, ocrMsgHandle_t**),
                                               delegateCommPollMessage);
    base->apiFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommApi_t*, ocrMsgHandle_t**),
                                               delegateCommWaitMessage);

    return base;
}

#endif /* ENABLE_COMM_API_DELEGATE */
