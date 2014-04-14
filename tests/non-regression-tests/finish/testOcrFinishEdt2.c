/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ocr.h"

#define FLAGS 0xdead
#define N 16

/**
 * DESC: Creates a top-level finish-edt which forks 16 edts, writing in a
 * shared data block. Then the sink edt checks everybody wrote to the db and
 * terminates.
 */

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

ocrGuid_t computeEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
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
        ocrEdtCreate(&updaterEdtGuid, updaterEdtTemplateGuid, EDT_PARAM_DEF, nparamv, EDT_PARAM_DEF, &(depv[0].guid), 0, NULL_GUID, NULL_GUID);
        i++;
    }
    return depv[0].guid;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t finishEdtOutputEventGuid;
    ocrGuid_t computeEdtGuid;
    ocrGuid_t computeEdtTemplateGuid;
    ocrEdtTemplateCreate(&computeEdtTemplateGuid, computeEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&computeEdtGuid, computeEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/ EDT_PROP_FINISH, NULL_GUID, /*outEvent=*/&finishEdtOutputEventGuid);

    // Build a data-block to be shared with sub-edts
    u64 * array;
    ocrGuid_t dbGuid;
    ocrDbCreate(&dbGuid,(void **) &array, sizeof(u64)*N, FLAGS, NULL_GUID, NO_ALLOC);

    ocrGuid_t terminateEdtGuid;
    ocrGuid_t terminateEdtTemplateGuid;
    ocrEdtTemplateCreate(&terminateEdtTemplateGuid, terminateEdt, 0 /*paramc*/, 2 /*depc*/);
    ocrEdtCreate(&terminateEdtGuid, terminateEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);
    ocrAddDependence(dbGuid, terminateEdtGuid, 0, DB_MODE_RO);
    ocrAddDependence(finishEdtOutputEventGuid, terminateEdtGuid, 1, DB_MODE_RO);

    // Use an event to channel the db guid to the main edt
    // Could also pass it directly as a depv
    ocrGuid_t dbEventGuid;
    ocrEventCreate(&dbEventGuid, OCR_EVENT_STICKY_T, true);

    ocrAddDependence(dbEventGuid, computeEdtGuid, 0, DB_MODE_RO);

    ocrEventSatisfy(dbEventGuid, dbGuid);

    return NULL_GUID;
}
