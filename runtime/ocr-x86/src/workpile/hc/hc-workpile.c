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
    base->pd->pdFree(base->pd, derived->deque);
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
    base->pd->processMessage(base->pd, &msg, false);
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
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) runtimeChunkAlloc(sizeof(ocrWorkpileHc_t), NULL);
    ocrWorkpile_t * base = (ocrWorkpile_t *) derived;

    base->fguid.guid = UNINITIALIZED_GUID;
    base->fguid.metaDataPtr = base;
    base->pd = NULL;
    base->fcts = factory->workpileFcts;
    return base;
}

/******************************************************/
/* OCR-HC WorkPile Factory                            */
/******************************************************/

void destructWorkpileFactoryHc(ocrWorkpileFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrWorkpileFactory_t * newOcrWorkpileFactoryHc(ocrParamList_t *perType) {
    ocrWorkpileFactoryHc_t* derived = (ocrWorkpileFactoryHc_t*)runtimeChunkAlloc(sizeof(ocrWorkpileFactoryHc_t), (void *)1);
    ocrWorkpileFactory_t* base = (ocrWorkpileFactory_t*) derived;
    base->instantiate = newWorkpileHc;
    base->destruct = destructWorkpileFactoryHc;
    base->workpileFcts.destruct = hcWorkpileDestruct;
    base->workpileFcts.begin = hcWorkpileBegin;
    base->workpileFcts.start = hcWorkpileStart;
    base->workpileFcts.stop = hcWorkpileStop;
    base->workpileFcts.finish = hcWorkpileFinish;
    base->workpileFcts.pop = hcWorkpilePop;
    base->workpileFcts.push = hcWorkpilePush;
    return base;
}
#endif /* ENABLE_WORKPILE_HC */
