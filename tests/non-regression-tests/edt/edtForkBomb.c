/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <ocr.h>

//#define N 7
#define N 6

ocrGuid_t done(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("done\n");
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t dec(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t event = paramv[0];
    ocrEventSatisfySlot(event, NULL_GUID, OCR_EVENT_LATCH_DECR_SLOT);
    return NULL_GUID;
}

/* Recursively call spawn(N-1) N times, and then call dec() */
ocrGuid_t spawn(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t edt;
    ocrGuid_t event  = paramv[0];
    ocrGuid_t tspawn = paramv[1];
    ocrGuid_t tdec   = paramv[2];
    int n            = paramv[3];
    int i;
    for(i = 0; i < n; i++) {
        u64 args[] = { event, tspawn, tdec, n-1 };
        ocrEventSatisfySlot(event, NULL_GUID, OCR_EVENT_LATCH_INCR_SLOT);
        ocrEdtCreate(&edt, tspawn, 4, args, 0, NULL, EDT_PROP_NONE, NULL_GUID, NULL);
    }
    ocrEdtCreate(&edt, tdec, 1, (u64*)&event, 0, NULL, EDT_PROP_NONE, NULL_GUID, NULL);
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t event;
    ocrGuid_t tspawn;
    ocrGuid_t tdec;
    ocrGuid_t tdone;
    ocrGuid_t edt;
    ocrEventCreate(&event, OCR_EVENT_LATCH_T, 0);
    ocrEventSatisfySlot(event, NULL_GUID, OCR_EVENT_LATCH_INCR_SLOT);
    ocrEdtTemplateCreate(&tspawn, spawn, 4, 0);
    ocrEdtTemplateCreate(&tdec  , dec  , 1, 0);
    ocrEdtTemplateCreate(&tdone , done , 0, 1);
    u64 args[] = { event, tspawn, tdec, N };
    ocrEdtCreate(&edt, tdone , 0, NULL, 1, &event, EDT_PROP_NONE, NULL_GUID, NULL);
    ocrEdtCreate(&edt, tspawn, 4, args, 0, NULL  , EDT_PROP_NONE, NULL_GUID, NULL);

    return NULL_GUID;
}
