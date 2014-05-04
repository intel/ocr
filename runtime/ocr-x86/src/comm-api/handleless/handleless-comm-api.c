/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_API_HANDLELESS

#include "debug.h"
#include "ocr-errors.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-worker.h"
#include "ocr-comm-platform.h"
#include "utils/ocr-utils.h"
#include "handleless-comm-api.h"

#define DEBUG_TYPE COMM_API

void handlelessCommDestruct (ocrCommApi_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void handlelessCommBegin(ocrCommApi_t* commApi, ocrPolicyDomain_t * PD) {
    commApi->pd = PD;
    commApi->commPlatform->fcts.begin(commApi->commPlatform, PD, commApi);
    return;
}

void handlelessCommStart(ocrCommApi_t * commApi, ocrPolicyDomain_t * PD) {
    commApi->commPlatform->fcts.start(commApi->commPlatform, PD, commApi);
    return;
}

void handlelessCommStop(ocrCommApi_t * commApi) {
    commApi->commPlatform->fcts.stop(commApi->commPlatform);
    return;
}

void handlelessCommFinish(ocrCommApi_t *commApi) {
    commApi->commPlatform->fcts.finish(commApi->commPlatform);
    return;
}

u8 handlelessCommSendMessage(ocrCommApi_t *self, ocrLocation_t target, ocrPolicyMsg_t *message,
                             ocrMsgHandle_t **handle, u32 properties) {
    u64 id;
    u64 bufSz = sizeof(ocrPolicyMsg_t);
    u64 bufferSize = bufSz | (bufSz << 32);
    if (message->type & PD_MSG_REQUEST) {
        ASSERT(!(message->type & PD_MSG_RESPONSE));
        if(handle) {
            ASSERT(message->type & PD_MSG_REQ_RESPONSE);
            ocrCommApiHandleless_t * commApiHandleless = (ocrCommApiHandleless_t*)self;
            ASSERT(commApiHandleless->handle.status == 0);
            *handle = &(commApiHandleless->handle);
            (*handle)->msg = message;
            (*handle)->response = NULL;
            (*handle)->status = HDL_NORMAL;
        }
    } else {
        ASSERT(message->type & PD_MSG_RESPONSE);
        ASSERT(!handle);
    }
    return self->commPlatform->fcts.sendMessage(self->commPlatform, target, message, bufferSize, &id, properties, 0);
}

u8 handlelessCommPollMessage(ocrCommApi_t *self, ocrMsgHandle_t **handle) {
    ASSERT(0); //TODO
    return OCR_ENOTSUP;
}

u8 handlelessCommWaitMessage(ocrCommApi_t *self, ocrMsgHandle_t **handle) {
    ASSERT(handle);
    ocrCommApiHandleless_t * commApiHandleless = (ocrCommApiHandleless_t*)self;
    u64 bufferSize = (u32)(sizeof(ocrPolicyMsg_t)) | (sizeof(ocrPolicyMsg_t) << 32);
    if (!(*handle)) {
        *handle = &(commApiHandleless->handle);
        ASSERT((*handle)->status == 0);
        (*handle)->status = HDL_NORMAL;
    } else {
        ASSERT((*handle)->msg);
    }
    ASSERT((*handle)->status == HDL_NORMAL &&
           (*handle) == (&(commApiHandleless->handle)));
    // Pass a "hint" saying that the the buffer is available
    (*handle)->response = (*handle)->msg;
    RESULT_ASSERT(self->commPlatform->fcts.waitMessage(
                      self->commPlatform, &((*handle)->response), &bufferSize, 0,
                      NULL), ==, 0);
    if((*handle)->response == (*handle)->msg) {
        // This means that the comm platform did *not* allocate the buffer itself
        (*handle)->properties = 0; // Indicates "do not free"
    } else {
        // This means that the comm platform did allocate the buffer itself
        (*handle)->properties = 1; // Indicates "do free"
    }
    return 0;
}

void handlelessCommDestructHandle(ocrMsgHandle_t *handle) {
    if(handle->properties == 1) {
        RESULT_ASSERT(handle->commApi->commPlatform->fcts.destructMessage(
                          handle->commApi->commPlatform, handle->response), ==, 0);
    }
    handle->msg = NULL;
    handle->response = NULL;
    handle->status = 0;
    handle->properties = 0;
}

ocrCommApi_t* newCommApiHandleless(ocrCommApiFactory_t *factory,
                                   ocrParamList_t *perInstance) {

    ocrCommApiHandleless_t * commApiHandleless = (ocrCommApiHandleless_t*)
        runtimeChunkAlloc(sizeof(ocrCommApiHandleless_t), NULL);
    factory->initialize(factory, (ocrCommApi_t*) commApiHandleless, perInstance);
    commApiHandleless->handle.msg = NULL;
    commApiHandleless->handle.response = NULL;
    commApiHandleless->handle.status = 0;
    commApiHandleless->handle.properties = 0;
    commApiHandleless->handle.commApi = (ocrCommApi_t*)commApiHandleless;
    commApiHandleless->handle.destruct = FUNC_ADDR(void (*)(ocrMsgHandle_t*), handlelessCommDestructHandle);

    return (ocrCommApi_t*)commApiHandleless;
}

/**
 * @brief Initialize an instance of comm-api handleless
 */
void initializeCommApiHandleless(ocrCommApiFactory_t * factory, ocrCommApi_t* self, ocrParamList_t * perInstance) {
    initializeCommApiOcr(factory, self, perInstance);
}

/******************************************************/
/* OCR COMM API HANDLELESS                            */
/******************************************************/

void destructCommApiFactoryHandleless(ocrCommApiFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCommApiFactory_t *newCommApiFactoryHandleless(ocrParamList_t *perType) {
    ocrCommApiFactory_t *base = (ocrCommApiFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCommApiFactoryHandleless_t), (void *)1);

    base->instantiate = newCommApiHandleless;
    base->initialize = initializeCommApiHandleless;
    base->destruct = destructCommApiFactoryHandleless;
    base->apiFcts.destruct = FUNC_ADDR(void (*)(ocrCommApi_t*), handlelessCommDestruct);
    base->apiFcts.begin = FUNC_ADDR(void (*)(ocrCommApi_t*, ocrPolicyDomain_t*),
                                    handlelessCommBegin);
    base->apiFcts.start = FUNC_ADDR(void (*)(ocrCommApi_t*, ocrPolicyDomain_t*),
                                    handlelessCommStart);
    base->apiFcts.stop = FUNC_ADDR(void (*)(ocrCommApi_t*), handlelessCommStop);
    base->apiFcts.finish = FUNC_ADDR(void (*)(ocrCommApi_t*), handlelessCommFinish);
    base->apiFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommApi_t*, ocrLocation_t, ocrPolicyMsg_t *, ocrMsgHandle_t**, u32),
                                          handlelessCommSendMessage);
    base->apiFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommApi_t*, ocrMsgHandle_t**),
                                          handlelessCommPollMessage);
    base->apiFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommApi_t*, ocrMsgHandle_t**),
                                          handlelessCommWaitMessage);

    return base;
}
#endif /* ENABLE_COMM_API_HANDLELESS */
