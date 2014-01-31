/*
 * This file is subject to the license agreement located in the file LIXENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_WORKPILE_XE

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-workpile.h"
#include "workpile/xe/xe-workpile.h"


/******************************************************/
/* OCR-XE WorkPile                                    */
/******************************************************/

void xeWorkpileDestruct ( ocrWorkpile_t * base ) {
    runtimeChunkFree((u64)base, NULL);
}

void xeWorkpileBegin(ocrWorkpile_t *base, ocrPolicyDomain_t *PD) {
    // Nothing to do
}

void xeWorkpileStart(ocrWorkpile_t *base, ocrPolicyDomain_t *PD) {
}

void xeWorkpileStop(ocrWorkpile_t *base) {
    base->fguid.guid = UNINITIALIZED_GUID;
}

void xeWorkpileFinish(ocrWorkpile_t *base) {
    // Nothing to do
}

ocrFatGuid_t xeWorkpilePop(ocrWorkpile_t * base, ocrWorkPopType_t type,
                           ocrCost_t *cost) {
    ocrFatGuid_t guid = {UNINITIALIZED_GUID, NULL};
    return guid;
}

void xeWorkpilePush(ocrWorkpile_t * base, ocrWorkPushType_t type,
                    ocrFatGuid_t g ) {
}

ocrWorkpile_t * newWorkpileXe(ocrWorkpileFactory_t * factory, ocrParamList_t *perInstanxe) {
    ocrWorkpileXe_t* derived = (ocrWorkpileXe_t*) runtimeChunkAlloc(sizeof(ocrWorkpileXe_t), NULL);
    ocrWorkpile_t * base = (ocrWorkpile_t *) derived;

    base->fguid.guid = UNINITIALIZED_GUID;
    base->fguid.metaDataPtr = base;
    base->pd = NULL;
    base->fcts = factory->workpileFcts;
    return base;
}

/******************************************************/
/* OCR-XE WorkPile Factory                            */
/******************************************************/

void destructWorkpileFactoryXe(ocrWorkpileFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrWorkpileFactory_t * newOcrWorkpileFactoryXe(ocrParamList_t *perType) {
    ocrWorkpileFactoryXe_t* derived = (ocrWorkpileFactoryXe_t*)runtimeChunkAlloc(sizeof(ocrWorkpileFactoryXe_t), NULL);
    ocrWorkpileFactory_t* base = (ocrWorkpileFactory_t*) derived;
    base->instantiate = newWorkpileXe;
    base->destruct = destructWorkpileFactoryXe;
    base->workpileFcts.destruct = xeWorkpileDestruct;
    base->workpileFcts.begin = xeWorkpileBegin;
    base->workpileFcts.start = xeWorkpileStart;
    base->workpileFcts.stop = xeWorkpileStop;
    base->workpileFcts.finish = xeWorkpileFinish;
    base->workpileFcts.pop = xeWorkpilePop;
    base->workpileFcts.push = xeWorkpilePush;
    return base;
}
#endif /* ENABLE_WORKPILE_XE */
