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

#include "fsim.h"

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
    return (ocrGuid_t) deque_non_competing_pop_head(derived->deque);
}

static void ceMessageWorkpilePush (ocrWorkpile_t * base, ocrGuid_t g ) {
    ocrWorkpileFsimMessage_t* derived = (ocrWorkpileFsimMessage_t*) base;
    deque_locked_push(derived->deque, (void *)g);
}

ocrWorkpile_t * newWorkpileFsimMessage(ocrWorkpileFactory_t * factory, ocrParamList_t *perInstance) {
    ocrWorkpileFsimMessage_t* derived = (ocrWorkpileFsimMessage_t*) malloc(sizeof(ocrWorkpileFsimMessage_t));
    ocrWorkpile_t * base = (ocrWorkpile_t *) derived;
    ocrMappable_t * module_base = (ocrMappable_t *) base;
    module_base->mapFct = NULL;
    base->fctPtrs = &(factory->workpileFcts);
    derived->deque = (mpsc_deque_t *) malloc(sizeof(mpsc_deque_t));
    mpscDequeInit(derived->deque, (void *) NULL_GUID);
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
