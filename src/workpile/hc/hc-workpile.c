/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-macros.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-workpile.h"
#include "workpile/hc/hc-workpile.h"


/******************************************************/
/* OCR-HC WorkPile                                    */
/******************************************************/

static void hcWorkpileDestruct ( ocrWorkpile_t * base ) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    dequeDestroy(derived->deque);
    free(derived);
}

static void hcWorkpileStart(ocrWorkpile_t *base, ocrPolicyDomain_t *PD) {
}

static void hcWorkpileStop(ocrWorkpile_t *base) {
}

static ocrGuid_t hcWorkpilePop ( ocrWorkpile_t * base, ocrCost_t *cost ) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    return (ocrGuid_t) dequePop(derived->deque);
}

static void hcWorkpilePush (ocrWorkpile_t * base, ocrGuid_t g ) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    dequePush(derived->deque, (void *)g);
}

static ocrGuid_t hcWorkpileSteal ( ocrWorkpile_t * base, ocrCost_t *cost ) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    return (ocrGuid_t) deque_steal(derived->deque);
}

static ocrWorkpile_t * newWorkpileHc(ocrWorkpileFactory_t * factory, ocrParamList_t *perInstance) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) checkedMalloc(derived, sizeof(ocrWorkpileHc_t));
    ocrWorkpile_t * base = (ocrWorkpile_t *) derived;
    ocrMappable_t * module_base = (ocrMappable_t *) base;
    module_base->mapFct = NULL;
    base->fctPtrs = &(factory->workpileFcts);
    derived->deque = (deque_t *) checkedMalloc(derived->deque, sizeof(deque_t));
    dequeInit(derived->deque, (void *) NULL_GUID);
    return base;
}


/******************************************************/
/* OCR-HC WorkPile Factory                            */
/******************************************************/

static void destructWorkpileFactoryHc(ocrWorkpileFactory_t * factory) {
    free(factory);
}

ocrWorkpileFactory_t * newOcrWorkpileFactoryHc(ocrParamList_t *perType) {
    ocrWorkpileFactoryHc_t* derived = (ocrWorkpileFactoryHc_t*) checkedMalloc(derived, sizeof(ocrWorkpileFactoryHc_t));
    ocrWorkpileFactory_t* base = (ocrWorkpileFactory_t*) derived;
    base->instantiate = newWorkpileHc;
    base->destruct =  destructWorkpileFactoryHc;
    base->workpileFcts.destruct = hcWorkpileDestruct;
    base->workpileFcts.start = hcWorkpileStart;
    base->workpileFcts.stop = hcWorkpileStop;
    base->workpileFcts.pop = hcWorkpilePop;
    base->workpileFcts.push = hcWorkpilePush;
    base->workpileFcts.steal = hcWorkpileSteal;
    return base;
}
