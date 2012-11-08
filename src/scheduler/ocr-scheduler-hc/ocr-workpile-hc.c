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

#include "hc.h"


/******************************************************/
/* OCR-HC WorkPool                                    */
/******************************************************/

static void hc_workpile_create ( ocr_workpile_t * base, void * configuration) {
    hc_workpile* derived = (hc_workpile*) base;
    derived->deque = (deque_t *) malloc(sizeof(deque_t));
    deque_init(derived->deque, (void *) NULL_GUID);
}

static void hc_workpile_destruct ( ocr_workpile_t * base ) {
    hc_workpile* derived = (hc_workpile*) base;
    free(derived->deque);
    free(derived);
}

static ocrGuid_t hc_workpile_pop ( ocr_workpile_t * base ) {
    hc_workpile* derived = (hc_workpile*) base;
    return (ocrGuid_t) deque_pop(derived->deque);
}

static void hc_workpile_push (ocr_workpile_t * base, ocrGuid_t g ) {
    hc_workpile* derived = (hc_workpile*) base;
    deque_push(derived->deque, (void *)g);
}

static ocrGuid_t hc_workpile_steal ( ocr_workpile_t * base ) {
    hc_workpile* derived = (hc_workpile*) base;
    return (ocrGuid_t) deque_steal(derived->deque);
}

ocr_workpile_t * hc_workpile_constructor(void) {
    hc_workpile* derived = (hc_workpile*) malloc(sizeof(hc_workpile));
    ocr_workpile_t * base = (ocr_workpile_t *) derived;
    base->create = hc_workpile_create;
    base->destruct = hc_workpile_destruct;
    base->pop = hc_workpile_pop;
    base->push = hc_workpile_push;
    base->steal = hc_workpile_steal;
    return base;
}
