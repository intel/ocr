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

#include "ocr-macros.h"
#include "ocr-types.h"
#include "ocr-workpile.h"

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
    u64 current = base->curr;
    ocrWorkpile_t * toBeReturned = base->array[current];
    base->curr = (current+1) % base->mod;
    return toBeReturned;
}

ocrWorkpileIterator_t* workpileIteratorConstructor ( u64 id, u64 workpileCount, ocrWorkpile_t ** workpiles ) {
    ocrWorkpileIterator_t* it = (ocrWorkpileIterator_t *) checkedMalloc(it, sizeof(ocrWorkpileIterator_t));
    it->array = workpiles;
    it->id = id;
    it->mod = workpileCount;
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
