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
#include "hc.h"


/******************************************************/
/* OCR-HC WorkPile Factory                            */
/******************************************************/

// Fwd declaration
ocrWorkpile_t* newWorkpileHc(ocrWorkpileFactory_t * factory, void * perTypeConfig, void * perInstanceConfig);

void destructWorkpileFactoryHc(ocrWorkpileFactory_t * factory) {
    free(factory);
}

ocrWorkpileFactory_t * newOcrWorkpileFactoryHc(void * config) {
    ocrWorkpileFactoryHc_t* derived = (ocrWorkpileFactoryHc_t*) checkedMalloc(derived, sizeof(ocrWorkpileFactoryHc_t));
    ocrWorkpileFactory_t* base = (ocrWorkpileFactory_t*) derived;
    base->instantiate = newWorkpileHc;
    base->destruct =  destructWorkpileFactoryHc;
    return base;
}

/******************************************************/
/* OCR-HC WorkPile                                    */
/******************************************************/

static void hc_workpile_destruct ( ocrWorkpile_t * base ) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    free(derived->deque);
    free(derived);
}

static ocrGuid_t hc_workpile_pop ( ocrWorkpile_t * base ) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    return (ocrGuid_t) dequePop(derived->deque);
}

static void hc_workpile_push (ocrWorkpile_t * base, ocrGuid_t g ) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    dequePush(derived->deque, (void *)g);
}

static ocrGuid_t hc_workpile_steal ( ocrWorkpile_t * base ) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) base;
    return (ocrGuid_t) deque_steal(derived->deque);
}

ocrWorkpile_t * newWorkpileHc(ocrWorkpileFactory_t * factory, void * perTypeConfig, void * perInstanceConfig) {
    ocrWorkpileHc_t* derived = (ocrWorkpileHc_t*) checkedMalloc(derived, sizeof(ocrWorkpileHc_t));
    ocrWorkpile_t * base = (ocrWorkpile_t *) derived;
    ocrMappable_t * module_base = (ocrMappable_t *) base;
    module_base->mapFct = NULL;
    base->destruct = hc_workpile_destruct;
    base->pop = hc_workpile_pop;
    base->push = hc_workpile_push;
    base->steal = hc_workpile_steal;
    derived->deque = (deque_t *) checkedMalloc(derived->deque, sizeof(deque_t));
    dequeInit(derived->deque, (void *) NULL_GUID);
    return base;
}
