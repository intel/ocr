/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */



#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - create an EDT at the same affinity a DB is using the query API.
 */

ocrGuid_t remoteEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // We ask the runtime to put the EDT in the same affinity group the DB was
    // Now we want to double check it was indeed the case by comparing the
    // current affinity of the EDT and the DB affinity we had stored.
    ocrGuid_t * dbPtr = depv[0].ptr;
    ocrGuid_t currentAffinity;
    ocrAffinityGetCurrent(&currentAffinity);
    PRINTF("remoteEdt: executing at affinity %lld\n", (u64) currentAffinity);

    ocrGuid_t queriedAffAffinityGuid;
    u64 count = 1;
    ocrAffinityQuery(dbPtr[0], &count, &queriedAffAffinityGuid);
    ASSERT(queriedAffAffinityGuid == currentAffinity);

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

    // Create a data-block and store its affinity inside for future cross-check
    ocrGuid_t dbGuid;
    ocrGuid_t * dbPtr;
    ocrDbCreate(&dbGuid, (void **)&dbPtr, sizeof(int) * affinityCount, DB_PROP_SINGLE_ASSIGNMENT, NULL_GUID, NO_ALLOC);
    dbPtr[0] = edtAffinity;
    ocrDbRelease(dbGuid);

    ocrGuid_t remoteEdtTemplateGuid;
    ocrEdtTemplateCreate(&remoteEdtTemplateGuid, remoteEdt, 0, 1);

    // Want to spawn the EDT in the same affinity group the DB is.
    ocrGuid_t queriedDbAffinityGuid;
    u64 count = 1;
    ocrAffinityQuery(dbGuid, &count, &queriedDbAffinityGuid);
    PRINTF("mainEdt: create remote EDT @ same affinity the DB was created\n");
    ocrGuid_t edtGuid;
    ocrEdtCreate(&edtGuid, remoteEdtTemplateGuid, EDT_PARAM_DEF, NULL, EDT_PARAM_DEF, &dbGuid,
        EDT_PROP_NONE, queriedDbAffinityGuid, NULL);

    return NULL_GUID;
}
