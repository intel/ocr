/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_WORKPILE_CE

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-workpile.h"
#include "workpile/ce/ce-workpile.h"


/******************************************************/
/* OCR-CE WorkPile                                    */
/******************************************************/

void ceWorkpileDestruct ( ocrWorkpile_t * base ) {
    ocrWorkpileCe_t* derived = (ocrWorkpileCe_t*) base;
    derived->deque->destruct(base->pd, derived->deque);
    base->pd->pdFree(base->pd, derived->deque);
    runtimeChunkFree((u64)base, NULL);
}

void ceWorkpileStart(ocrWorkpile_t *base, ocrPolicyDomain_t *PD) {
    guidify(PD, (u64)base, &(base->fguid), OCR_GUID_WORKPILE);
    ocrWorkpileCe_t* derived = (ocrWorkpileCe_t*)base;
    base->pd = PD;
    derived->deque = newNonConcurrentQueue(base->pd, (void *) NULL_GUID);
}

void ceWorkpileStop(ocrWorkpile_t *base) {
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

void ceWorkpileFinish(ocrWorkpile_t *base) {
    // Nothing to do
}

ocrFatGuid_t ceWorkpilePop(ocrWorkpile_t * base, ocrWorkPopType_t type,
                           ocrCost_t *cost) {
    
    ocrWorkpileCe_t* derived = (ocrWorkpileCe_t*) base;
    ocrFatGuid_t fguid;
    switch(type) {
    case POP_WORKPOPTYPE:
        fguid.guid = (ocrGuid_t)derived->deque->pop_from_head(derived->deque, 0); 
        break;
    default:
        ASSERT(0);
    }
    fguid.metaDataPtr = NULL_GUID;
    return fguid;
}

void ceWorkpilePush(ocrWorkpile_t * base, ocrWorkPushType_t type,
                    ocrFatGuid_t g ) {
    ocrWorkpileCe_t* derived = (ocrWorkpileCe_t*) base;
    derived->deque->push_at_tail(derived->deque, (void *)(g.guid), 0);
}

ocrWorkpile_t * newWorkpileCe(ocrWorkpileFactory_t * factory, ocrParamList_t *perInstance) {
    ocrWorkpileCe_t* derived = (ocrWorkpileCe_t*) runtimeChunkAlloc(sizeof(ocrWorkpileCe_t), NULL);
    ocrWorkpile_t * base = (ocrWorkpile_t *) derived;

    base->fguid.guid = UNINITIALIZED_GUID;
    base->fguid.metaDataPtr = base;
    base->pd = NULL;
    base->fcts = factory->workpileFcts;
    derived->deque = NULL;
    return base;
}

/******************************************************/
/* OCR-CE WorkPile Factory                            */
/******************************************************/

void destructWorkpileFactoryCe(ocrWorkpileFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrWorkpileFactory_t * newOcrWorkpileFactoryCe(ocrParamList_t *perType) {
    ocrWorkpileFactoryCe_t* derived = (ocrWorkpileFactoryCe_t*)runtimeChunkAlloc(sizeof(ocrWorkpileFactoryCe_t), NULL);
    ocrWorkpileFactory_t* base = (ocrWorkpileFactory_t*) derived;
    base->instantiate = newWorkpileCe;
    base->destruct = destructWorkpileFactoryCe;
    base->workpileFcts.destruct = ceWorkpileDestruct;
    base->workpileFcts.start = ceWorkpileStart;
    base->workpileFcts.stop = ceWorkpileStop;
    base->workpileFcts.finish = ceWorkpileFinish;
    base->workpileFcts.pop = ceWorkpilePop;
    base->workpileFcts.push = ceWorkpilePush;
    return base;
}
#endif /* ENABLE_WORKPILE_CE */
