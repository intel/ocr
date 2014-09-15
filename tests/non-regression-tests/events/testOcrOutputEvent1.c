/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */





#include "ocr.h"

/**
 * DESC: Chain an edt to another edt's output event and read its output value
 */

// This edt is triggered when the output event of the other edt is satisfied by the runtime
ocrGuid_t chainedEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int * val = (int *) depv[0].ptr;
    ASSERT((*val) == 42);
    ocrShutdown(); // This is the last EDT to execute, terminate
    return NULL_GUID;
}

ocrGuid_t taskForEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Build a data-block and return its guid
    int *k;
    ocrGuid_t dbGuid;
    ocrDbCreate(&dbGuid,(void **) &k, sizeof(int), /*flags=*/0,
                /*location=*/NULL_GUID, NO_ALLOC);
    *k = 42;
    // It will be channeled through the output event declared in
    // ocrCreateEdt to any entity depending on that event.
    return dbGuid;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Setup output event
    ocrGuid_t outputEvent;
    ocrGuid_t edtGuid;
    ocrGuid_t taskForEdtTemplateGuid;
    ocrEdtTemplateCreate(&taskForEdtTemplateGuid, taskForEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&edtGuid, taskForEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/0, NULL_GUID, /*outEvent=*/&outputEvent);

    // Create the chained EDT and add input and output events as dependences.
    ocrGuid_t chainedEdtGuid;
    ocrGuid_t chainedEdtTemplateGuid;
    ocrEdtTemplateCreate(&chainedEdtTemplateGuid, chainedEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&chainedEdtGuid, chainedEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/ 0, NULL_GUID, /*outEvent=*/NULL_GUID);
    ocrAddDependence(outputEvent, chainedEdtGuid, 0, DB_MODE_RO);

    // Start the first EDT
    ocrAddDependence(NULL_GUID, edtGuid, 0, DB_DEFAULT_MODE);

    return NULL_GUID;
}
