/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */




#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - create two remote EDT with != EDT_PARAM_UNK paramc (+ a lot more for the sink EDT)
 */

ocrGuid_t remoteEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    if(paramc == 2) {
        ASSERT(paramv[0] == 555);
        ASSERT(paramv[1] == 666);
        PRINTF("[remote] RemoteEdt: 2 paramv checked\n");
        ocrEventSatisfy(((ocrGuid_t *) depv[0].ptr)[0], NULL_GUID);
    }
    if(paramc == 4) {
        ASSERT(paramv[0] == 111);
        ASSERT(paramv[1] == 222);
        ASSERT(paramv[2] == 333);
        ASSERT(paramv[3] == 444);
        PRINTF("[remote] RemoteEdt: 4 paramv checked\n");
        ocrEventSatisfy(((ocrGuid_t *) depv[0].ptr)[1], NULL_GUID);
    }
    return NULL_GUID;
}

ocrGuid_t shutdownEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ASSERT(affinityCount >= 1);
    ocrGuid_t affinities[affinityCount];
    ocrAffinityGet(AFFINITY_PD, &affinityCount, affinities);
    ocrGuid_t edtAffinity = affinities[affinityCount-1];

    // Setup the sink EDT
    ocrGuid_t event1Guid;
    ocrEventCreate(&event1Guid, OCR_EVENT_STICKY_T, false);
    ocrGuid_t event2Guid;
    ocrEventCreate(&event2Guid, OCR_EVENT_STICKY_T, false);
    ocrGuid_t dbGuid;
    ocrGuid_t * dbPtr;
    ocrDbCreate(&dbGuid, (void **)&dbPtr, sizeof(ocrGuid_t) * 2, DB_PROP_SINGLE_ASSIGNMENT, NULL_GUID, NO_ALLOC);
    dbPtr[0] = event1Guid;
    dbPtr[1] = event2Guid;
    ocrDbRelease(dbGuid);
    ocrGuid_t shutdownEdtTemplateGuid;
    ocrEdtTemplateCreate(&shutdownEdtTemplateGuid, shutdownEdt, 0, 2);
    ocrGuid_t edtShutdownGuid;
    ocrGuid_t shutdownDepv[2] = {event1Guid, event2Guid};
    ocrEdtCreate(&edtShutdownGuid, shutdownEdtTemplateGuid, 0, NULL, 2, shutdownDepv,
        EDT_PROP_NONE, NULL_GUID, NULL);

    // Setup the test
    ocrGuid_t remoteEdtTemplateGuid;
    ocrEdtTemplateCreate(&remoteEdtTemplateGuid, remoteEdt, EDT_PARAM_UNK, 1);
    ocrGuid_t edt1Guid;
    u64 edtParamv1[2] = {555,666};
    ocrEdtCreate(&edt1Guid, remoteEdtTemplateGuid, 2, (u64*) &edtParamv1, 1, &dbGuid,
        EDT_PROP_NONE, edtAffinity, NULL);
    ocrGuid_t edt2Guid;
    u64 edtParamv2[4] = {111,222,333,444};
    ocrEdtCreate(&edt2Guid, remoteEdtTemplateGuid, 4, (u64*) &edtParamv2, 1, &dbGuid,
        EDT_PROP_NONE, edtAffinity, NULL);

    return NULL_GUID;
}
