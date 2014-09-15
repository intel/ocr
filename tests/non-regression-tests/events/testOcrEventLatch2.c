/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */





#include "ocr.h"

/**
 * DESC: latch-event simulating a finish-edt + check termination of children
 * Does not work yet in distributed because of multiple IW on the DB.
 */

#define N 16

ocrGuid_t terminateEDT(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("Call Terminate\n");
    int * array = (int*) depv[1].ptr;
    int i = 0;
    while (i < N) {
        ASSERT(array[i] == 1);
        i++;
    }
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t childEDT(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t latchGuid = *((ocrGuid_t*) depv[0].ptr);
    int idx = *((int*) depv[1].ptr);
    int * array = (int*) depv[2].ptr;

    // Do some work
    array[idx] = 1;

    // Checkout of the finish scope
    ocrEventSatisfySlot(latchGuid, NULL_GUID, OCR_EVENT_LATCH_DECR_SLOT);
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Create a latch
    ocrGuid_t latchGuid;
    ocrEventCreate(&latchGuid, OCR_EVENT_LATCH_T, false);

    // Create a datablock to wrap the latch's guid
    ocrGuid_t *dbLatchPtr;
    ocrGuid_t dbLatchGuid;
    ocrDbCreate(&dbLatchGuid,(void **)&dbLatchPtr, sizeof(ocrGuid_t), 0, NULL_GUID, NO_ALLOC);
    *dbLatchPtr = latchGuid;
    ocrDbRelease(dbLatchGuid);

    // Create an array children can write to so that the sink edt
    // can check each child did a write at their index.
    int *arrayPtr;
    ocrGuid_t arrayGuid;
    ocrDbCreate(&arrayGuid,(void **)&arrayPtr, sizeof(int)*N, 0, NULL_GUID, NO_ALLOC);

    // Set up the sink edt, activated by the latch satisfaction
    ocrGuid_t terminateEDTGuid;
    ocrGuid_t terminateEdtTemplateGuid;
    ocrEdtTemplateCreate(&terminateEdtTemplateGuid, terminateEDT, 0 /*paramc*/, 2 /*depc*/);
    ocrEdtCreate(&terminateEDTGuid, terminateEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);
    ocrAddDependence(latchGuid, terminateEDTGuid, 0, DB_MODE_RO);
    ocrAddDependence(arrayGuid, terminateEDTGuid, 1, DB_MODE_RO);

    // Check in the current finish scope
    ocrEventSatisfySlot(latchGuid, NULL_GUID, OCR_EVENT_LATCH_INCR_SLOT);

    // Spawn a number of children, that will checkin/checkout of the scope too
    int i =0;
    while(i < N) {
        // Check child in the current finish scope
        ocrEventSatisfySlot(latchGuid, NULL_GUID, OCR_EVENT_LATCH_INCR_SLOT);

        int *idxPtr;
        ocrGuid_t idxGuid;
        ocrDbCreate(&idxGuid,(void **)&idxPtr, sizeof(int), 0, NULL_GUID, NO_ALLOC);
        *idxPtr = i;

        ocrGuid_t childEdtGuid;
        ocrGuid_t childEdtTemplateGuid;
        ocrEdtTemplateCreate(&childEdtTemplateGuid, childEDT, 0 /*paramc*/, 3 /*depc*/);
        ocrEdtCreate(&childEdtGuid, childEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                     /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

        ocrAddDependence(dbLatchGuid, childEdtGuid, 0, DB_MODE_RO);
        ocrAddDependence(idxGuid, childEdtGuid, 1, DB_MODE_RO);
        ocrAddDependence(arrayGuid, childEdtGuid, 2, DB_MODE_ITW);

        i++;
    }

    ocrEventSatisfySlot(latchGuid, NULL_GUID, OCR_EVENT_LATCH_DECR_SLOT);
    return NULL_GUID;
}
