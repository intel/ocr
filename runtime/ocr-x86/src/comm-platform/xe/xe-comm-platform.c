/*
 * This file is subject to the lixense agreement located in the file LICENSE
 * and cannot be distributed without it. This notixe cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_XE

#include "debug.h"

#include "ocr-comm-api.h"
#include "ocr-policy-domain.h"

#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"

#include "xe-comm-platform.h"

#define DEBUG_TYPE COMM_PLATFORM

void xeCommDestruct (ocrCommPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void xeCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t *api) {
}

void xeCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t *api) {
}

void xeCommStop(ocrCommPlatform_t * commPlatform) {
}

void xeCommFinish(ocrCommPlatform_t *commPlatform) {
}

u8 xeCommSetMaxExpectedMessageSize(ocrCommPlatform_t *self, u64 size, u32 mask) {
    ASSERT(0);
    return 0;
}

u8 xeCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target,
                     ocrPolicyMsg_t *message, u64 bufferSize, u64 *id,
                     u32 properties, u32 mask) {
    //TODO: Send an alarm to the CE
    ASSERT(0);
    return 0;
}

u8 xeCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                     u32 properties, u32 *mask) {
    ASSERT(0);
    return 0;
}

u8 xeCommWaitMessage(ocrCommPlatform_t *self,  ocrPolicyMsg_t **msg,
                     u32 properties, u32 *mask) {
    ASSERT(0);
    return 0;
}

u8 xeCommDestructMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t *msg) {
    ASSERT(0);
    return 0;
}

ocrCommPlatform_t* newCommPlatformXe(ocrCommPlatformFactory_t *factory,
                                     ocrParamList_t *perInstance) {

    ocrCommPlatformXe_t * commPlatformXe = (ocrCommPlatformXe_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformXe_t), NULL);
    ocrCommPlatform_t * base = (ocrCommPlatform_t *) commPlatformXe;
    factory->initialize(factory, base, perInstance);
    return base;
}

void initializeCommPlatformXe(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * base, ocrParamList_t * perInstance) {
    initializeCommPlatformOcr(factory, base, perInstance);
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

void destructCommPlatformFactoryXe(ocrCommPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCommPlatformFactory_t *newCommPlatformFactoryXe(ocrParamList_t *perType) {
    ocrCommPlatformFactory_t *base = (ocrCommPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformFactoryXe_t), (void *)1);
    
    base->instantiate = &newCommPlatformXe;
    base->initialize = &initializeCommPlatformXe;
    base->destruct = &destructCommPlatformFactoryXe;

    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), xeCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrCommApi_t*), xeCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrCommApi_t*), xeCommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), xeCommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), xeCommFinish);
    base->platformFcts.setMaxExpectedMessageSize = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, u64, u32),
                                                             xeCommSetMaxExpectedMessageSize);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrLocation_t,
                                                      ocrPolicyMsg_t *, u64, u64*, u32, u32), xeCommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**, u32, u32*),
                                               xeCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**, u32, u32*),
                                               xeCommWaitMessage);
    base->platformFcts.destructMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t*),
                                                   xeCommDestructMessage);

    return base;
}
#endif /* ENABLE_COMM_PLATFORM_XE */
