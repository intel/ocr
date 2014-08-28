/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_NULL

#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-errors.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"
#include "null-comm-platform.h"

#define DEBUG_TYPE COMM_PLATFORM

void nullCommDestruct (ocrCommPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void nullCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t *comm) {
    // We are a NULL communication so we don't do anything
    return;
}

void nullCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t *comm) {
    return;
}

void nullCommStop(ocrCommPlatform_t * commPlatform) {
    // Nothing to do really
}

void nullCommFinish(ocrCommPlatform_t *commPlatform) {
}

u8 nullCommSetMaxExpectedMessageSize(ocrCommPlatform_t *self, u64 size, u32 mask) {
    ASSERT(0);
    return OCR_ENOTSUP;
}

u8 nullCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target,
                       ocrPolicyMsg_t *message, u64 bufferSize, u64 *id,
                       u32 properties, u32 mask) {
    ASSERT(0);
    return OCR_ENOTSUP;
}

u8 nullCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                       u64 *bufferSize, u32 properties, u32 *mask) {
    ASSERT(0);
    return OCR_ENOTSUP;
}

u8 nullCommWaitMessage(ocrCommPlatform_t *self,  ocrPolicyMsg_t **msg,
                       u64 *bufferSize, u32 properties, u32 *mask) {
    ASSERT(0);
    return OCR_ENOTSUP;
}

u8 nullCommDestructMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t *msg) {
    ASSERT(0);
    return OCR_ENOTSUP;
}

ocrCommPlatform_t* newCommPlatformNull(ocrCommPlatformFactory_t *factory,
                                       ocrParamList_t *perInstance) {

    ocrCommPlatformNull_t * commPlatformNull = (ocrCommPlatformNull_t*)
            runtimeChunkAlloc(sizeof(ocrCommPlatformNull_t), PERSISTENT_CHUNK);
    ocrCommPlatform_t * derived = (ocrCommPlatform_t *) commPlatformNull;
    factory->initialize(factory, derived, perInstance);
    return derived;
}

void initializeCommPlatformNull(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * derived, ocrParamList_t * perInstance) {
    initializeCommPlatformOcr(factory, derived, perInstance);
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

void destructCommPlatformFactoryNull(ocrCommPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCommPlatformFactory_t *newCommPlatformFactoryNull(ocrParamList_t *perType) {
    ocrCommPlatformFactory_t *base = (ocrCommPlatformFactory_t*)
                                     runtimeChunkAlloc(sizeof(ocrCommPlatformFactoryNull_t), NONPERSISTENT_CHUNK);

    base->instantiate = &newCommPlatformNull;
    base->initialize = &initializeCommPlatformNull;
    base->destruct = &destructCommPlatformFactoryNull;

    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), nullCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                         ocrCommApi_t*), nullCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                         ocrCommApi_t*), nullCommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), nullCommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), nullCommFinish);
    base->platformFcts.setMaxExpectedMessageSize = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, u64, u32),
            nullCommSetMaxExpectedMessageSize);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrLocation_t,
                                     ocrPolicyMsg_t *, u64, u64*, u32, u32), nullCommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**, u64*, u32, u32*),
                                     nullCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**, u64*, u32, u32*),
                                     nullCommWaitMessage);
    base->platformFcts.destructMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t*),
                                         nullCommDestructMessage);


    return base;
}
#endif /* ENABLE_COMM_PLATFORM_NULL */
