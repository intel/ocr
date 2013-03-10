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
 * DESC: Test addDependence(db, event, slot); Satisfies event, triggers edt.
 */

ocrGuid_t task_for_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int* res = (int*)depv[0].ptr;
    printf("In the task_for_edt with value %d\n", (*res));
    assert(*res == 42);
    // This is the last EDT to execute, terminate
    ocrFinish();
    return NULL_GUID;
}

int main (int argc, char ** argv) {
    ocrEdt_t fctPtrArray [1];
    fctPtrArray[0] = &task_for_edt;
    ocrInit(&argc, argv, 1, fctPtrArray);

    // Creates a data block
    int *k;
    ocrGuid_t dbGuid;
    ocrDbCreate(&dbGuid,(void **) &k,
            sizeof(int), /*flags=*/0,
            /*location=*/NULL,
            NO_ALLOC);
    *k = 42;

    ocrGuid_t eventGuid;
    ocrEventCreate(&eventGuid, OCR_EVENT_STICKY_T, true);

    // Creates the EDT
    ocrGuid_t edtGuid;
    ocrEdtCreate(&edtGuid, task_for_edt, /*paramc=*/0, /*params=*/ NULL,
            /*paramv=*/NULL, /*properties=*/0,
            /*depc=*/1, /*depv=*/NULL, /*outEvent=*/NULL_GUID);

    ocrAddDependence(eventGuid, edtGuid, 0);

    // Schedule the EDT (will run when dependences satisfied)
    ocrEdtSchedule(edtGuid);

    // Register a dependence between a db and an event
    // (equivalent to directly satisfying the DB)
    ocrAddDependence(dbGuid, eventGuid, 0);

    // No need to satisfy as addDependence is equivalent to a satisfy
    // when the source is a datablock 
    ocrCleanup();

    return 0;
}