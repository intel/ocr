/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-macros.h"
#include "ocr-types.h"
#include "ocr-workpile.h"

#include <stdlib.h>

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

ocrWorkpileIterator_t* newWorkpileIterator ( u64 id, u64 workpileCount, ocrWorkpile_t ** workpiles ) {
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

void workpileIteratorDestruct (ocrWorkpileIterator_t* base) {
    free(base);
}
