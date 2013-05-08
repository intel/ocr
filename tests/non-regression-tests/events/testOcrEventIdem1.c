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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ocr.h"

/**
 * DESC: Satisfy an 'idempotent' event several times (subsequent ignored)
 */

ocrGuid_t computeEdt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int* res = (int*)depv[0].ptr;
    printf("In the task_for_edt with value %d\n", (*res));
    assert(*res == 42);
    // This is the last EDT to execute, terminate
    ocrFinish();
    return NULL_GUID;
}

int main (int argc, char ** argv) {
    ocrEdt_t fctPtrArray [1];
    fctPtrArray[0] = &computeEdt;
    ocrInit(&argc, argv, 1, fctPtrArray);

    // Current thread is '0' and goes on with user code.
    ocrGuid_t e0;
    ocrEventCreate(&e0, OCR_EVENT_IDEM_T, true);
    ocrGuid_t e1;
    ocrEventCreate(&e1, OCR_EVENT_IDEM_T, true);

    // Creates the EDT
    ocrGuid_t edtGuid;

    ocrEdtCreate(&edtGuid, computeEdt, /*paramc=*/0, /*params=*/ NULL,
            /*paramv=*/NULL, /*properties=*/0,
            /*depc=*/2, /*depv=*/NULL, /*outEvent=*/NULL_GUID);

    // Register a dependence between an event and an edt
    ocrAddDependence(e0, edtGuid, 0);
    ocrAddDependence(e1, edtGuid, 1);

    // Schedule the EDT (will run when dependences satisfied)
    ocrEdtSchedule(edtGuid);

    int *k0;
    ocrGuid_t dbGuid0;
    ocrDbCreate(&dbGuid0,(void **) &k0, sizeof(int), /*flags=*/0,
            /*location=*/NULL, NO_ALLOC);
    *k0 = 42;

    int *k1;
    ocrGuid_t dbGuid1;
    ocrDbCreate(&dbGuid1,(void **) &k1, sizeof(int), /*flags=*/0,
            /*location=*/NULL, NO_ALLOC);
    *k1 = 43;

    // Satify first slot with db pointing to '42'
    ocrEventSatisfy(e0, dbGuid0);

    // These should be ignored by the runtime and the db shouldn't be updated
    ocrEventSatisfy(e0, dbGuid1);
    ocrEventSatisfy(e0, dbGuid1);

    // Trigger the edt
    ocrEventSatisfy(e1, NULL_GUID);

    ocrCleanup();

    return 0;
}
