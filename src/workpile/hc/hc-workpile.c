/* Copyright (c) 2012, Rice University

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1.  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2.  Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
   3.  Neither the name of Intel Corporation
   nor the names of its contributors may be used to endorse or
   promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
