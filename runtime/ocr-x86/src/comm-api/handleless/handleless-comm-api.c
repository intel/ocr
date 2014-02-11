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
    if (*handle) {
        ASSERT((*handle)->status == HDL_NORMAL && (*handle)->msg && (*handle) == (&(commApiHandleless->handle)));
        RESULT_ASSERT(self->commPlatform->fcts.waitMessage(self->commPlatform, &((*handle)->response), 0, NULL), ==, 0);
    } else {
        *handle = &(commApiHandleless->handle);
        ASSERT((*handle)->status == 0);
        (*handle)->status = HDL_NORMAL;
        RESULT_ASSERT(self->commPlatform->fcts.waitMessage(self->commPlatform, &((*handle)->msg), 0, NULL), ==, 0);
    }
    return 0;
}

void handlelessCommDestructHandle(ocrMsgHandle_t *handle) {
    handle->msg = NULL;
    handle->response = NULL;
    handle->status = 0;
}

ocrCommApi_t* newCommApiHandleless(ocrCommApiFactory_t *factory,
                                   ocrParamList_t *perInstance) {

    ocrCommApiHandleless_t * commApiHandleless = (ocrCommApiHandleless_t*)
        runtimeChunkAlloc(sizeof(ocrCommApiHandleless_t), NULL);

    
    commApiHandleless->base.fcts = factory->apiFcts;
    commApiHandleless->handle.msg = NULL;
    commApiHandleless->handle.response = NULL;
    commApiHandleless->handle.status = 0;
    commApiHandleless->handle.destruct = FUNC_ADDR(void (*)(ocrMsgHandle_t*), handlelessCommDestructHandle);

    return (ocrCommApi_t*)commApiHandleless;
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
