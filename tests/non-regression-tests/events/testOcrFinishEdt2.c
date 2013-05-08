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

#define FLAGS 0xdead
#define N 100

/**
 * DESC: Creates a top-level finish-edt which forks 100 edts, writing in a 
 * shared data block. Then the sink edt checks everybody wrote to the db and
 * terminates.
 */

// This edt is triggered when the output event of the other edt is satisfied by the runtime
ocrGuid_t terminate_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // TODO shouldn't be doing that... but need more support from output events to get a 'return' value from an edt
    assert(depv[0].guid != NULL_GUID);
    int * array = (int*)depv[0].ptr;
    int i = 0;
    while (i < N) {
        assert(array[i] == i);
        i++;
    }
    ocrFinish(); // This is the last EDT to execute, terminate
    return NULL_GUID;
}

ocrGuid_t updater_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // Retrieve id 
    assert(paramc == 2);
    assert(params[0] == (u64) sizeof(int));
    assert(params[1] == (u64) sizeof(ocrGuid_t));
    int id = *((int *) paramv[0]);
    assert ((id>=0) && (id < N));
    ocrGuid_t dbGuid = (ocrGuid_t) paramv[1];
    // Since we passed the db guid through paramv, we need to acquire/release // it properly. This would have been done automatically through depv.
    int * dbPtr;
    ocrDbAcquire(dbGuid, (void **) &dbPtr, 0);
    dbPtr[id] = id;
    ocrDbRelease(dbGuid);
    free(params);
    free(paramv);
    // When we are done we return the guid to the array
    // so that the chained edt can retrieve the db.
    return NULL_GUID;
}

ocrGuid_t main_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int i = 0;
    while (i < N) {
        // Pass down the index to write to and the db guid through params
        // (Could also be done through dependences)
        u32 nparamc = 2;
        u64 * nparams = (u64*) malloc(sizeof(u64)*nparamc);
        nparams[0] = (u64) sizeof(int);
        nparams[1] = (u64) sizeof(ocrGuid_t);
        int * idx = (int *) malloc(sizeof(int));
        *idx = i;
        void ** nparamv = (void **) malloc(sizeof(void *)*nparamc);
        nparamv[0] = (void *) idx;
        nparamv[1] = (void *) depv[0].guid;

        ocrGuid_t updaterEdtGuid;
        ocrEdtCreate(&updaterEdtGuid, updater_edt, nparamc, nparams, nparamv, 0, 0, NULL, NULL_GUID);
        ocrEdtSchedule(updaterEdtGuid);
        i++;
    }
    return depv[0].guid;
}

int main (int argc, char ** argv) {
    ocrEdt_t fctPtrArray [3];
    fctPtrArray[0] = &main_edt;
    fctPtrArray[1] = &updater_edt;
    fctPtrArray[2] = &terminate_edt;
    ocrInit(&argc, argv, 3, fctPtrArray);

    ocrGuid_t finishEdtOutputEventGuid;
    ocrGuid_t mainEdtGuid;
    ocrEdtCreate(&mainEdtGuid, main_edt, /*paramc=*/0, /*params=*/ NULL,
            /*paramv=*/NULL, /*properties=*/ EDT_PROP_FINISH, /*depc=*/1, NULL, &finishEdtOutputEventGuid);

    // Build a data-block to be shared with sub-edts
    int * array;
    ocrGuid_t dbGuid;
    ocrDbCreate(&dbGuid,(void **) &array, sizeof(int)*N, FLAGS, NULL, NO_ALLOC);

    ocrGuid_t terminateEdtGuid;
    ocrEdtCreate(&terminateEdtGuid, terminate_edt, /*paramc=*/0, /*params=*/ NULL, /*paramv=*/NULL, /*properties=*/0, /*depc=*/1, /*depv=*/NULL, NULL_GUID);
    ocrAddDependence(finishEdtOutputEventGuid, terminateEdtGuid, 0);
    //ocrAddDependence(dbGuid, terminateEdtGuid, 1);
    ocrEdtSchedule(terminateEdtGuid);
    
    // Use an event to channel the db guid to the main edt
    // Could also pass it directly as a depv
    ocrGuid_t dbEventGuid;
    ocrEventCreate(&dbEventGuid, OCR_EVENT_STICKY_T, true);

    ocrAddDependence(dbEventGuid, mainEdtGuid, 0);
    
    ocrEdtSchedule(mainEdtGuid);

    ocrEventSatisfy(dbEventGuid, dbGuid);

    ocrCleanup();

    return 0;
}
