/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <ocr.h>

#define N 4

ocrGuid_t done(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("done\n");
    ocrShutdown();
    return NULL_GUID;
}

/* Recursively call spawn(N-1) N times, and then call dec() */
ocrGuid_t spawn(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t edt;
    ocrGuid_t tspawn = paramv[0];
    int n            = paramv[1];
    int i;
    for(i = 0; i < n; i++) {
        u64 args[] = {tspawn, n-1 };
        ocrGuid_t noGuid = NULL_GUID;
        ocrEdtCreate(&edt, tspawn, 2, args, 1, &noGuid, EDT_PROP_NONE, NULL_GUID, NULL);
    }
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t tspawn;
    ocrGuid_t tdone;
    ocrGuid_t edtDone;
    ocrGuid_t edtHead;
    ocrEdtTemplateCreate(&tspawn, spawn, 2, 1);
    ocrEdtTemplateCreate(&tdone , done , 0, 1);
    u64 args[] = {tspawn, N};
    ocrGuid_t eventDone;
    ocrEdtCreate(&edtHead, tspawn, 2, args, 1, NULL, EDT_PROP_FINISH, NULL_GUID, &eventDone);
    ocrEdtCreate(&edtDone, tdone , 0, NULL, 1, &eventDone, EDT_PROP_NONE, NULL_GUID, NULL);
    // Triggers the head spawn edt
    ocrAddDependence(NULL_GUID, edtHead, 0, DB_MODE_RO);
    return NULL_GUID;
}
