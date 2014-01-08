/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMP_TARGET_HC

#include "comp-target/hc/hc-comp-target.h"
#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-comp-target.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif


void hcDestruct(ocrCompTarget_t *compTarget) {
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

void hcStart(ocrCompTarget_t * compTarget, ocrPolicyDomain_t * PD, launchArg_t * launchArg) {
    // Get a GUID
    guidify(PD, (u64)compTarget, &(compTarget->fguid), OCR_GUID_COMPTARGET);
    compTarget->pd = PD;

#ifdef OCR_ENABLE_STATISTICS
    statsCOMPTARGET_START(PD, compTarget->fguid.guid, compTarget->fguid.metaDataPtr);
#endif

    ASSERT(compTarget->platformCount == 1);
    compTarget->platforms[0]->fcts.start(compTarget->platforms[0], PD, launchArg);
}

void hcStop(ocrCompTarget_t * compTarget) {
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
    compTarget->pd->processMessage(compTarget->pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
    compTarget->fguid.guid = UNINITIALIZED_GUID;
}

void hcFinish(ocrCompTarget_t *compTarget) {
    ASSERT(compTarget->platformCount == 1);
    compTarget->platforms[0]->fcts.finish(compTarget->platforms[0]);
}

u8 hcGetThrottle(ocrCompTarget_t *compTarget, u64 *value) {
    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.getThrottle(compTarget->platforms[0], value);
}

u8 hcSetThrottle(ocrCompTarget_t *compTarget, u64 value) {
    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.setThrottle(compTarget->platforms[0], value);
}

u8 hcSendMessage(ocrCompTarget_t *compTarget, ocrPhysicalLocation_t target,
                 ocrPolicyMsg_t **message) {
    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.sendMessage(compTarget->platforms[0], target, message);
}

u8 hcPollMessage(ocrCompTarget_t *compTarget, ocrPolicyMsg_t **message) {
    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.pollMessage(compTarget->platforms[0], message);
}

u8 hcWaitMessage(ocrCompTarget_t *compTarget, ocrPolicyMsg_t **message) {
    ASSERT(compTarget->platformCount == 1);
    return compTarget->platforms[0]->fcts.waitMessage(compTarget->platforms[0], message);
}

ocrCompTarget_t * newCompTargetHc(ocrCompTargetFactory_t * factory,
                                  ocrPhysicalLocation_t location,
                                  ocrParamList_t* perInstance) {
    ocrCompTargetHc_t * compTarget = (ocrCompTargetHc_t*)runtimeChunkAlloc(sizeof(ocrCompTargetHc_t), NULL);

    compTarget->base.fguid.guid = UNINITIALIZED_GUID;
    compTarget->base.fguid.metaDataPtr = compTarget;
    compTarget->base.location = location;
    
    compTarget->base.platforms = NULL;
    compTarget->base.platformCount = 0;
    compTarget->base.fcts = factory->targetFcts;

    // TODO: Setup routine and routineArg. Should be in perInstance misc
    
    return (ocrCompTarget_t*)compTarget;
}

/******************************************************/
/* OCR COMP TARGET HC FACTORY                         */
/******************************************************/
static void destructCompTargetFactoryHc(ocrCompTargetFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCompTargetFactory_t *newCompTargetFactoryHc(ocrParamList_t *perType) {
    ocrCompTargetFactory_t *base = (ocrCompTargetFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCompTargetFactoryHc_t), NULL);
    base->instantiate = &newCompTargetHc;
    base->destruct = &destructCompTargetFactoryHc;
    base->targetFcts.destruct = &hcDestruct;
    base->targetFcts.start = &hcStart;
    base->targetFcts.stop = &hcStop;
    base->targetFcts.finish = &hcFinish;
    base->targetFcts.getThrottle = &hcGetThrottle;
    base->targetFcts.setThrottle = &hcSetThrottle;
    base->targetFcts.sendMessage = &hcSendMessage;
    base->targetFcts.pollMessage = &hcPollMessage;
    base->targetFcts.waitMessage = &hcWaitMessage;

    return base;
}

#endif /* ENABLE_COMP_TARGET_HC */
