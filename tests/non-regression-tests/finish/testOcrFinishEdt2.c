/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */



#include "ocr.h"

#define N 64

/**
 * DESC: Creates a top-level finish-edt which forks 64 edts. No correctness check done.
 */

// This edt is triggered when the output event of the other edt is satisfied by the runtime
ocrGuid_t terminateEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrShutdown(); // This is the last EDT to execute, terminate
    return NULL_GUID;
}

ocrGuid_t updaterEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // PRINTF("Run updaterEdt %d\n", (int)paramv[0]);
    return NULL_GUID;
}

ocrGuid_t computeEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t updaterEdtTemplateGuid;
    ocrEdtTemplateCreate(&updaterEdtTemplateGuid, updaterEdt, 1 /*paramc*/, 0/*depc*/);
    u64 i = 0;
    while (i < N) {
        ocrGuid_t updaterEdtGuid;
        u64 nparamv = i;
        ocrEdtCreate(&updaterEdtGuid, updaterEdtTemplateGuid, EDT_PARAM_DEF, &nparamv, EDT_PARAM_DEF, NULL, 0, NULL_GUID, NULL_GUID);
        i++;
    }
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t finishEdtOutputEventGuid;
    ocrGuid_t computeEdtGuid;
    ocrGuid_t computeEdtTemplateGuid;
    ocrEdtTemplateCreate(&computeEdtTemplateGuid, computeEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&computeEdtGuid, computeEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/ EDT_PROP_FINISH, NULL_GUID, /*outEvent=*/&finishEdtOutputEventGuid);

    ocrGuid_t terminateEdtGuid;
    ocrGuid_t terminateEdtTemplateGuid;
    ocrEdtTemplateCreate(&terminateEdtTemplateGuid, terminateEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&terminateEdtGuid, terminateEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);
    ocrAddDependence(finishEdtOutputEventGuid, terminateEdtGuid, 0, DB_MODE_RO);
    // Triggers the finish EDT
    ocrAddDependence(NULL_GUID, computeEdtGuid, 0, DB_MODE_RO);

    return NULL_GUID;
}
