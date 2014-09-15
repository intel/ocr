/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */




#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - Create finish EDT affinitized to single affinity (remote but not distributed)
 * Note: In this test affinities are a must not a may.
 */

#define COUNT_EDT 10

ocrGuid_t shutdownEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("shutdownEdt: executing\n");
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t remoteEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("RemoteEdt: executing\n");
    ocrGuid_t eventGuid = ((ocrGuid_t *) depv[0].ptr)[0];
    ocrEventSatisfy(eventGuid, NULL_GUID);
    return NULL_GUID;
}

ocrGuid_t finishEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t edtTemplateGuid;
    ocrEdtTemplateCreate(&edtTemplateGuid, remoteEdt, 0, 1);

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
    ocrGuid_t currentAffinity;
    ocrAffinityGetCurrent(&currentAffinity);
    // Spawn 'COUNT_EDT' EDTs, each satisfying its event
    i = 0;
    while(i < COUNT_EDT) {
        // Create EDTs
        ocrGuid_t edtGuid;
        ocrEdtCreate(&edtGuid, edtTemplateGuid, 0, NULL, EDT_PARAM_DEF, &dbsGuid[i],
            EDT_PROP_NONE, currentAffinity, NULL);
        i++;
    }
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ocrGuid_t dbAffGuid;
    ocrGuid_t * dbAffPtr;
    ocrDbCreate(&dbAffGuid, (void **)&dbAffPtr, sizeof(ocrGuid_t) * affinityCount, DB_PROP_SINGLE_ASSIGNMENT, NULL_GUID, NO_ALLOC);
    ocrAffinityGet(AFFINITY_PD, &affinityCount, dbAffPtr);
    ASSERT(affinityCount >= 1);
    ocrGuid_t affinityGuid = dbAffPtr[affinityCount-1];
    ocrDbRelease(dbAffGuid);

    // Finish EDT potentially remote, but spawning all children on the same node
    ocrGuid_t finishEdtTemplateGuid;
    ocrEdtTemplateCreate(&finishEdtTemplateGuid, finishEdt, 0, 0);

    PRINTF("mainEdt: spawning remote finish-EDT\n");
    ocrGuid_t edtGuid;
    ocrEdtCreate(&edtGuid, finishEdtTemplateGuid, 0, NULL, EDT_PARAM_DEF, 0,
        EDT_PROP_NONE, affinityGuid, NULL);

    return NULL_GUID;
}
