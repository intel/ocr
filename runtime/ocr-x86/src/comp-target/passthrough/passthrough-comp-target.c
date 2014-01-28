/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMP_TARGET_PASSTHROUGH

#include "comp-target/passthrough/passthrough-comp-target.h"
#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-comp-target.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif


void ptDestruct(ocrCompTarget_t *compTarget) {
    u32 i = 0;
    while(i < compTarget->platformCount) {
        compTarget->platforms[i]->fcts.destruct(compTarget->platforms[i]);
        ++i;
    }
    runtimeChunkFree((u64)(compTarget->platforms), NULL);
#ifdef OCR_ENABLE_STATISTICS
    statsCOMPTARGET_STOP(compTarget->pd, compTarget->fguid.guid, compTarget);
#endif
    runtimeChunkFree((u64)compTarget, NULL);
}

void ptBegin(ocrCompTarget_t * compTarget, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    
    ASSERT(compTarget->platformCount == 1);
    compTarget->pd = PD;
    compTarget->platforms[0]->fcts.begin(compTarget->platforms[0], PD, workerType);
}

void ptStart(ocrCompTarget_t * compTarget, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType,
             launchArg_t * launchArg) {
    // Get a GUID
    guidify(PD, (u64)compTarget, &(compTarget->fguid), OCR_GUID_COMPTARGET);

#ifdef OCR_ENABLE_STATISTICS
    statsCOMPTARGET_START(PD, compTarget->fguid.guid, compTarget->fguid.metaDataPtr);
#endif

    ASSERT(compTarget->platformCount == 1);
    compTarget->platforms[0]->fcts.start(compTarget->platforms[0], PD, workerType, launchArg);
}

void ptStop(ocrCompTarget_t * compTarget) {
    ASSERT(compTarget->platformCount == 1);
    compTarget->platforms[0]->fcts.stop(compTarget->platforms[0]);

    // Destroy the GUID
    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);
    
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid) = compTarget->fguid;
    PD_MSG_FIELD(properties) = 0;
    compTarget->pd->processMessage(compTarget->pd, &msg, false); // Don't really care about result
#undef PD_MSG
#undef PD_TYPE
    compTarget->fguid.guid = UNINITIALIZED_GUID;
}

void ptFinish(ocrCompTarget_t *compTarget) {
    ASSERT(compTarget->platformCount == 1);
    compTarget->platforms[0]->fcts.finish(compTarget->platforms[0]);
}

u8 ptGetThrottle(ocrCompTarget_t *compTarget, u64 *value) {
    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.getThrottle(compTarget->platforms[0], value);
}

u8 ptSetThrottle(ocrCompTarget_t *compTarget, u64 value) {
    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.setThrottle(compTarget->platforms[0], value);
}

u8 ptSendMessage(ocrCompTarget_t *compTarget, ocrLocation_t target,
                 ocrPolicyMsg_t **message) {
    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.sendMessage(compTarget->platforms[0], target, message);
}

u8 ptPollMessage(ocrCompTarget_t *compTarget, ocrPolicyMsg_t **message, u32 mask) {
    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.pollMessage(compTarget->platforms[0], message, mask);
}

u8 ptWaitMessage(ocrCompTarget_t *compTarget, ocrPolicyMsg_t **message) {
    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.waitMessage(compTarget->platforms[0], message);
}

u8 ptSetCurrentEnv(ocrCompTarget_t *compTarget, ocrPolicyDomain_t *pd,
                   ocrWorker_t *worker) {

    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.setCurrentEnv(compTarget->platforms[0], pd, worker);
}

ocrCompTarget_t * newCompTargetPt(ocrCompTargetFactory_t * factory,
                                  ocrParamList_t* perInstance) {
    ocrCompTargetPt_t * compTarget = (ocrCompTargetPt_t*)runtimeChunkAlloc(sizeof(ocrCompTargetPt_t), NULL);

    compTarget->base.fguid.guid = UNINITIALIZED_GUID;
    compTarget->base.fguid.metaDataPtr = compTarget;
    
    compTarget->base.platforms = NULL;
    compTarget->base.platformCount = 0;
    compTarget->base.fcts = factory->targetFcts;
    
    return (ocrCompTarget_t*)compTarget;
}

/******************************************************/
/* OCR COMP TARGET HC FACTORY                         */
/******************************************************/
static void destructCompTargetFactoryPt(ocrCompTargetFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCompTargetFactory_t *newCompTargetFactoryPt(ocrParamList_t *perType) {
    ocrCompTargetFactory_t *base = (ocrCompTargetFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCompTargetFactoryPt_t), (void *)1);
    base->instantiate = &newCompTargetPt;
    base->destruct = &destructCompTargetFactoryPt;
    base->targetFcts.destruct = &ptDestruct;
    base->targetFcts.begin = &ptBegin;
    base->targetFcts.start = &ptStart;
    base->targetFcts.stop = &ptStop;
    base->targetFcts.finish = &ptFinish;
    base->targetFcts.getThrottle = &ptGetThrottle;
    base->targetFcts.setThrottle = &ptSetThrottle;
    base->targetFcts.sendMessage = &ptSendMessage;
    base->targetFcts.pollMessage = &ptPollMessage;
    base->targetFcts.waitMessage = &ptWaitMessage;
    base->targetFcts.setCurrentEnv = &ptSetCurrentEnv;

    return base;
}

#endif /* ENABLE_COMP_TARGET_PT */

