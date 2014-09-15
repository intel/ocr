/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */




#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - create a remote EDT
 */

#define COUNT_EDT 10

ocrGuid_t shutdownEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("[remote] shutdownEdt: executing\n");
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t remoteEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("[remote] RemoteEdt: executing\n");
    ocrGuid_t eventGuid = *((ocrGuid_t *) depv[0].ptr);
    ocrEventSatisfy(eventGuid, NULL_GUID);
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // u64 affinityCount;
    // ocrAffinityCount(AFFINITY_PD, &affinityCount);
    // ocrGuid_t dbAffGuid;
    // ocrGuid_t * dbAffPtr;
    // ocrDbCreate(&dbAffGuid, (void **)&dbAffPtr, sizeof(ocrGuid_t) * affinityCount, DB_PROP_SINGLE_ASSIGNMENT, NULL_GUID, NO_ALLOC);
    // ocrAffinityGet(AFFINITY_PD, &affinityCount, dbAffPtr);
    // ocrDbRelease(dbAffGuid);

    // create local edt that depends on the remote edt, the db is automatically cloned
    ocrGuid_t remoteEdtTemplateGuid;
    ocrEdtTemplateCreate(&remoteEdtTemplateGuid, remoteEdt, 0, 1);

    ocrGuid_t eventsGuid[COUNT_EDT];
    ocrGuid_t dbsGuid[COUNT_EDT];
    u64 i = 0;
    while(i < COUNT_EDT) {
        ocrEventCreate(&eventsGuid[i], OCR_EVENT_STICKY_T, false);
        ocrGuid_t * dbPtr;
        ocrDbCreate(&dbsGuid[i], (void **)&dbPtr, sizeof(ocrGuid_t), DB_PROP_SINGLE_ASSIGNMENT, NULL_GUID, NO_ALLOC);
        dbPtr[0] = eventsGuid[i];
        ocrDbRelease(dbsGuid[i]);
        i++;
    }

    // Setup shutdown EDT
    ocrGuid_t shutdownEdtTemplateGuid;
    ocrEdtTemplateCreate(&shutdownEdtTemplateGuid, shutdownEdt, 0, COUNT_EDT);

    ocrGuid_t shutdownEdtGuid;
    ocrEdtCreate(&shutdownEdtGuid, shutdownEdtTemplateGuid, 0, NULL, EDT_PARAM_DEF, eventsGuid,
                 EDT_PROP_NONE, NULL_GUID, NULL);

    // Spawn an COUNT_EDT EDTs, each satisfying its event
    i = 0;
    while(i < COUNT_EDT) {
        // Create EDTs
        ocrGuid_t edtGuid;
        ocrEdtCreate(&edtGuid, remoteEdtTemplateGuid, 0, NULL, EDT_PARAM_DEF, &dbsGuid[i],
            EDT_PROP_NONE, NULL_GUID, NULL);
        i++;
    }

    return NULL_GUID;
}
