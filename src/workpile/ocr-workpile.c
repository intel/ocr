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

#include <stdlib.h>
#include <assert.h>

#include "ocr-macros.h"
#include "ocr-types.h"
#include "ocr-workpile.h"

//TODO don't like that: I think we should have some kind of a plugin
// mechanism where an implementation can associate its string to a function pointer.
// i.e HC_WORKPOOL -> newWorkpileHc
extern ocrWorkpile_t * newWorkpileHc(ocrWorkpileFactory_t * factory, void * perTypeConfig, void * perInstanceConfig);
extern ocrWorkpile_t * newWorkpileFsimMessage(ocrWorkpileFactory_t * factory, void * perTypeConfig, void * perInstanceConfig);

ocrWorkpile_t * newWorkpile(ocrWorkpileKind workpileType, void * perTypeConfig, void * perInstanceConfig) {
    switch(workpileType) {
    case OCR_DEQUE:
        return newWorkpileHc(NULL, perTypeConfig, perInstanceConfig);
    case OCR_MESSAGE_QUEUE:
        return newWorkpileFsimMessage(NULL, perTypeConfig, perInstanceConfig);
    default:
        assert(false && "Unrecognized workpile kind");
        break;
    }

    return NULL;
}


/******************************************************/
/* OCR Workpile iterator                              */
/******************************************************/

void workpileIteratorReset (ocrWorkpileIterator_t * base) {
    base->curr = ((base->id) + 1) % base->mod;
}

bool workpileIteratorHasNext (ocrWorkpileIterator_t * base) {
    return base->id != base->curr;
}

ocrWorkpile_t * workpileIteratorNext (ocrWorkpileIterator_t * base) {
    int current = base->curr;
    ocrWorkpile_t * toBeReturned = base->array[current];
    base->curr = (current+1) % base->mod;
    return toBeReturned;
}

ocrWorkpileIterator_t* workpileIteratorConstructor ( int i, u64 n_pools, ocrWorkpile_t ** pools ) {
    ocrWorkpileIterator_t* it = (ocrWorkpileIterator_t *) checkedMalloc(it, sizeof(ocrWorkpileIterator_t));
    it->array = pools;
    it->id = i;
    it->mod = n_pools;
    it->hasNext = workpileIteratorHasNext;
    it->next = workpileIteratorNext;
    it->reset = workpileIteratorReset;
    // The 'curr' field is initialized by reset
    it->reset(it);
    return it;
}

void workpile_iterator_destructor (ocrWorkpileIterator_t* base) {
    free(base);
}
