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
#define N 100
/**
 * DESC:
 */

// This edt is triggered when the output event of the other edt is satisfied by the runtime
ocrGuid_t terminateEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    printf("Terminate\n");
    ocrShutdown(); // This is the last EDT to execute, terminate
    return NULL_GUID;
}

ocrGuid_t userEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    printf("Running User EDT\n");
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t outputEventGuid;

    ocrGuid_t terminateEdtGuid;
    ocrGuid_t terminateEdtTemplateGuid;
    ocrEdtTemplateCreate(&terminateEdtTemplateGuid, terminateEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&terminateEdtGuid, terminateEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                    /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

    ocrGuid_t userEdtGuid;
    ocrGuid_t userEdtTemplateGuid;
    ocrEdtTemplateCreate(&userEdtTemplateGuid, userEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&userEdtGuid, userEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                    /*properties=*/EDT_PROP_FINISH, NULL_GUID, /*outEvent=*/&outputEventGuid);

    printf("Add dependence\n");
    ocrAddDependence(outputEventGuid, terminateEdtGuid, 0, DB_MODE_RO);

    // Now we start the userEdt (we start *after* adding the dependence)
    ocrAddDependence(NULL_GUID, userEdtGuid, 0, DB_DEFAULT_MODE);
    
    return NULL_GUID;
}
