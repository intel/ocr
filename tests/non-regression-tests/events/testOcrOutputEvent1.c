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
 * DESC: Chain an edt to another edt's output event and read its output value
 */

// This edt is triggered when the output event of the other edt is satisfied by the runtime
ocrGuid_t chainedEdt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int * val = (int *) depv[0].ptr;
    assert((*val) == 42);
    ocrFinish(); // This is the last EDT to execute, terminate
    return NULL_GUID;
}

ocrGuid_t mainEdt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // Build a data-block and return its guid
    int *k;
    ocrGuid_t dbGuid;
    ocrDbCreate(&dbGuid,(void **) &k, sizeof(int), /*flags=*/0, 
                /*location=*/NULL, NO_ALLOC);
    *k = 42;
    // It will be channeled through the output event declared in 
    // ocrCreateEdt to any entity depending on that event.
    return dbGuid;
}

int main (int argc, char ** argv) {
    ocrEdt_t fctPtrArray [2];
    fctPtrArray[0] = &mainEdt;
    fctPtrArray[1] = &chainedEdt;

    ocrInit(&argc, argv, 2, fctPtrArray);

    // Setup output event
    ocrGuid_t outputEvent;

    ocrGuid_t mainEdtGuid;
    ocrEdtCreate(&mainEdtGuid, mainEdt, /*paramc=*/0, /*params=*/ NULL,
            /*paramv=*/NULL, /*properties=*/0, /*depc=*/0, /*depv=*/NULL, &outputEvent);

    // Create the chained EDT and add input and output events as dependences.
    ocrGuid_t chainedEdtGuid;
    ocrEdtCreate(&chainedEdtGuid, chainedEdt, /*paramc=*/0, /*params=*/ NULL,
            /*paramv=*/NULL, /*properties=*/0, /*depc=*/1, /*depv=*/NULL, NULL_GUID);
    ocrAddDependence(outputEvent, chainedEdtGuid, 0);
    
    ocrEdtSchedule(chainedEdtGuid);
    ocrEdtSchedule(mainEdtGuid);    

    ocrCleanup();

    return 0;
}
