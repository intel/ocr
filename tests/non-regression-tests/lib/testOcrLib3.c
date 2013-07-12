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

#include "ocr.h"

// Only tested when OCR library interface is available
#ifdef OCR_LIBRARY_ITF

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ocr-lib.h"

/**
 * DESC: OCR-library, init, edt-finish, finalize
 * Builds a finish-edt that spawns a number of sub-edt to fill-in an array.
 * Then checks if all the elements of the array have been properly set.
 */

#define N 2


// This edt is triggered when the output event of the other edt is satisfied by the runtime
ocrGuid_t terminateEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // TODO shouldn't be doing that... but need more support from output events to get a 'return' value from an edt
    assert(depv[0].guid != NULL_GUID);
    u64 * array = (u64*)depv[0].ptr;
    u64 i = 0;
    while (i < N) {
        assert(array[i] == i);
        i++;
    }
    printf("Everything went OK\n");
    ocrShutdown(); // This is the last EDT to execute, terminate
    return NULL_GUID;
}

ocrGuid_t updaterEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Retrieve id 
    assert(paramc == 1);
    u64 id = paramv[0];
    assert ((id>=0) && (id < N));
    u64 * dbPtr = (u64 *) depv[0].ptr;
    dbPtr[id] = id;
    return NULL_GUID;
}

ocrGuid_t rootEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Build a data-block to be shared with sub-edts
    u64 * array;
    ocrGuid_t dbGuid;
    ocrDbCreate(&dbGuid,(void **) &array, sizeof(u64)*N, 0, NULL_GUID, NO_ALLOC);

    ocrGuid_t updaterEdtTemplateGuid;
    ocrEdtTemplateCreate(&updaterEdtTemplateGuid, updaterEdt, 1 /*paramc*/, 1/*depc*/);
    u64 i = 0;
    while (i < N) {
        // Pass down the index to write to and the db guid through params
        // (Could also be done through dependences)
        u32 nparamc = 1;
        u64* nparamv = (u64*) malloc(sizeof(void *)*nparamc);
        nparamv[0] = i;
        // Pass the guid we got fron depv to the updaterEdt through depv
        ocrGuid_t updaterEdtGuid;
        ocrEdtCreate(&updaterEdtGuid, updaterEdtTemplateGuid, EDT_PARAM_DEF, nparamv, EDT_PARAM_DEF, &dbGuid, 0, NULL_GUID, NULL_GUID);
        i++;
    }
    return dbGuid;
}

int main(int argc, const char * argv[]) {
    ocrConfig_t ocrConfig;
    ocrParseArgs(argc, argv, &ocrConfig);
    ocrInit(&ocrConfig);

    ocrGuid_t terminateEdtGuid;
    ocrGuid_t terminateEdtTemplateGuid;
    ocrEdtTemplateCreate(&terminateEdtTemplateGuid, terminateEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&terminateEdtGuid, terminateEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                    /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

    ocrGuid_t outputEventGuid;
    ocrGuid_t rootEdtGuid;
    ocrGuid_t rootEdtTemplateGuid;
    ocrEdtTemplateCreate(&rootEdtTemplateGuid, rootEdt, 0 /*paramc*/, 0 /*depc*/);
    ocrEdtCreate(&rootEdtGuid, rootEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                    /*properties=*/EDT_PROP_FINISH, NULL_GUID, /*outEvent=*/&outputEventGuid);
    ocrAddDependence(outputEventGuid, terminateEdtGuid, 0, DB_MODE_RO);

    // Wait for the output-event to be satisfied before proceeding
    ocrWait(outputEventGuid);

    ocrFinalize();
    return 0;
}

#else

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrShutdown();
    return NULL_GUID;
}

#endif
