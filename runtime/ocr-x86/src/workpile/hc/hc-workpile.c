/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_WORKPILE_HC

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-workpile.h"
#include "workpile/hc/hc-workpile.h"


/******************************************************/
/* OCR-HC WorkPile                                    */
/******************************************************/

void hcWorkpileDestruct ( ocrWorkpile_t * base ) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    derived->deque->destruct(base->pd, derived->deque);
    base->pd->fcts.pdFree(base->pd, derived->deque);
    runtimeChunkFree((u64)base, NULL);
}

void hcWorkpileBegin(ocrWorkpile_t *base, ocrPolicyDomain_t *PD) {
    // Nothing to do
}

void hcWorkpileStart(ocrWorkpile_t *base, ocrPolicyDomain_t *PD) {
    guidify(PD, (u64)base, &(base->fguid), OCR_GUID_WORKPILE);
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*)base;
    base->pd = PD;
    derived->deque = newWorkStealingDeque(base->pd, (void *) NULL_GUID);
}

void hcWorkpileStop(ocrWorkpile_t *base) {
    // Destroy the GUID
    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);
    
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid) = base->fguid;
    PD_MSG_FIELD(properties) = 0;
    // Shutting down so ignore error
    base->pd->fcts.processMessage(base->pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
    base->fguid.guid = UNINITIALIZED_GUID;
}

void hcWorkpileFinish(ocrWorkpile_t *base) {
    // Nothing to do
}

ocrFatGuid_t hcWorkpilePop(ocrWorkpile_t * base, ocrWorkPopType_t type,
                           ocrCost_t *cost) {
    
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    ocrFatGuid_t fguid;
    switch(type) {
    case POP_WORKPOPTYPE:
        fguid.guid = (ocrGuid_t)derived->deque->popFromTail(derived->deque, 0); 
        break;
    case STEAL_WORKPOPTYPE:
        fguid.guid = (ocrGuid_t)derived->deque->popFromHead(derived->deque, 1);
        break;
    default:
        ASSERT(0);
    }
    fguid.metaDataPtr = NULL_GUID;
    return fguid;
}

void hcWorkpilePush(ocrWorkpile_t * base, ocrWorkPushType_t type,
                    ocrFatGuid_t g ) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    derived->deque->pushAtTail(derived->deque, (void *)(g.guid), 0);
}

ocrWorkpile_t * newWorkpileHc(ocrWorkpileFactory_t * factory, ocrParamList_t *perInstance) {
    ocrWorkpile_t* derived = (ocrWorkpile_t*) runtimeChunkAlloc(sizeof(ocrWorkpile_t), NULL);
    factory->initialize(factory, derived, perInstance);
    return derived;
}

void initializeWorkpileHc(ocrWorkpileFactory_t * factory, ocrWorkpile_t* self, ocrParamList_t * perInstance) {
   initializeWorkpileOcr(factory, self, perInstance);
}

/******************************************************/
/* OCR-HC WorkPile Factory                            */
/******************************************************/

void destructWorkpileFactoryHc(ocrWorkpileFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrWorkpileFactory_t * newOcrWorkpileFactoryHc(ocrParamList_t *perType) {
    ocrWorkpileFactory_t* base = (ocrWorkpileFactory_t*)runtimeChunkAlloc(sizeof(ocrWorkpileFactoryHc_t), (void *)1);

    base->instantiate = &newWorkpileHc;
    base->initialize = &initializeWorkpileHc;
    base->destruct = &destructWorkpileFactoryHc;

    base->workpileFcts.destruct = FUNC_ADDR(void (*) (ocrWorkpile_t *), hcWorkpileDestruct);
    base->workpileFcts.begin = FUNC_ADDR(void (*) (ocrWorkpile_t *, ocrPolicyDomain_t *), hcWorkpileBegin);
    base->workpileFcts.start = FUNC_ADDR(void (*) (ocrWorkpile_t *, ocrPolicyDomain_t *), hcWorkpileStart);
    base->workpileFcts.stop = FUNC_ADDR(void (*) (ocrWorkpile_t *), hcWorkpileStop);
    base->workpileFcts.finish = FUNC_ADDR(void (*) (ocrWorkpile_t *), hcWorkpileFinish);
    base->workpileFcts.pop = FUNC_ADDR(ocrFatGuid_t (*)(ocrWorkpile_t*, ocrWorkPopType_t, ocrCost_t *), hcWorkpilePop);
    base->workpileFcts.push = FUNC_ADDR(void (*)(ocrWorkpile_t*, ocrWorkPushType_t, ocrFatGuid_t), hcWorkpilePush);
    return base;
}
#endif /* ENABLE_WORKPILE_HC */
