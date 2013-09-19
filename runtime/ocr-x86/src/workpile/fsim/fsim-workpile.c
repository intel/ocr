/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-macros.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-workpile.h"

#include "workpile/fsim/fsim-workpile.h"

/******************************************************/
/* OCR-FSIM-CE Message Workpile                       */
/******************************************************/

static void ceMessageWorkpileDestruct ( ocrWorkpile_t * base ) {
    ocrWorkpileFsimMessage_t* derived = (ocrWorkpileFsimMessage_t*) base;
    free(derived->deque);
    free(derived);
}

static ocrGuid_t ceMessageWorkpilePop ( ocrWorkpile_t * base, ocrCost_t *cost) {
    ocrWorkpileFsimMessage_t* derived = (ocrWorkpileFsimMessage_t*) base;
    return (ocrGuid_t) semiConcDequeNonLockedPop(derived->deque);
}

static void ceMessageWorkpilePush (ocrWorkpile_t * base, ocrGuid_t g ) {
    ocrWorkpileFsimMessage_t* derived = (ocrWorkpileFsimMessage_t*) base;
    semiConcDequeLockedPush(derived->deque, (void *)g);
}

ocrWorkpile_t * newWorkpileFsimMessage(ocrWorkpileFactory_t * factory, ocrParamList_t *perInstance) {
    ocrWorkpileFsimMessage_t* derived = (ocrWorkpileFsimMessage_t*) malloc(sizeof(ocrWorkpileFsimMessage_t));
    ocrWorkpile_t * base = (ocrWorkpile_t *) derived;
    ocrMappable_t * module_base = (ocrMappable_t *) base;
    module_base->mapFct = NULL;
    base->fctPtrs = &(factory->workpileFcts);
    derived->deque = (semiConcDeque_t *) malloc(sizeof(semiConcDeque_t));
    semiConcDequeInit(derived->deque, (void *) NULL_GUID);
    return base;
}


/******************************************************/
/* OCR-FSIM-CE Message Workpile Factory               */
/******************************************************/

void destructWorkpileFactoryFsimMessage(ocrWorkpileFactory_t * factory) {
    free(factory);
}

ocrWorkpileFactory_t * newOcrWorkpileFactoryFsimMessage(ocrParamList_t *perType) {
    ocrWorkpileFactoryFsimMessage_t* derived = (ocrWorkpileFactoryFsimMessage_t*) checkedMalloc(derived, sizeof(ocrWorkpileFactoryFsimMessage_t));
    ocrWorkpileFactory_t* base = (ocrWorkpileFactory_t*) derived;
    base->instantiate = newWorkpileFsimMessage;
    base->destruct =  destructWorkpileFactoryFsimMessage;
    base->workpileFcts.destruct = ceMessageWorkpileDestruct;
    base->workpileFcts.pop = ceMessageWorkpilePop;
    base->workpileFcts.push = ceMessageWorkpilePush;
    base->workpileFcts.steal = ceMessageWorkpilePop;
    return base;
}
