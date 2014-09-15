
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - Competing DB writer EDTs in EW mode (no early dbRelease)
 */

#define TYPE_ELEM_DB int

// BIG
#define NB_ELEM_DB 2000
#define NB_WRITERS 40

ocrGuid_t shutdownEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t eWriteEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    TYPE_ELEM_DB * data = (TYPE_ELEM_DB *) depv[0].ptr;
    // First check all DBs elements are equal
    TYPE_ELEM_DB c = data[0];
    u32 i = 0;
    while (i < NB_ELEM_DB) {
        ASSERT (data[i] == c);
        i++;
    }
    // Then write a new value
    TYPE_ELEM_DB v = (TYPE_ELEM_DB) paramv[0];
    i = 0;
    while (i < NB_ELEM_DB) {
        data[i] = v;
        i++;
    }
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ASSERT(affinityCount >= 1);
    ocrGuid_t affinities[affinityCount];
    ocrAffinityGet(AFFINITY_PD, &affinityCount, affinities);
    ocrGuid_t affinity = affinities[affinityCount-1];

    // Create DB @ affinity
    void * dbPtr;
    ocrGuid_t dbGuid;
    u32 nbElem = NB_ELEM_DB;
    ocrDbCreate(&dbGuid, &dbPtr, sizeof(TYPE_ELEM_DB) * NB_ELEM_DB, 0, affinity, NO_ALLOC);
    u64 i = 0;
    int * data = (int *) dbPtr;
    while (i < nbElem) {
        data[i] = -1;
        i++;
    }
    ocrDbRelease(dbGuid);

    ocrGuid_t shutdownEdtTemplateGuid;
    ocrEdtTemplateCreate(&shutdownEdtTemplateGuid, shutdownEdt, 0, NB_WRITERS);
    ocrGuid_t shutdownGuid;
    ocrEdtCreate(&shutdownGuid, shutdownEdtTemplateGuid, 0, NULL, NB_WRITERS, NULL,
                 EDT_PROP_NONE, NULL_GUID, NULL);

    // create writer EDT for the DB in EW mode @ affinity
    ocrGuid_t eWriteEdtTemplateGuid;
    ocrEdtTemplateCreate(&eWriteEdtTemplateGuid, eWriteEdt, 1, 1);

    i = 0;
    while (i < NB_WRITERS) {
        ocrGuid_t outputEventGuid;
        ocrGuid_t eWriteEdtGuid;
        ocrEdtCreate(&eWriteEdtGuid, eWriteEdtTemplateGuid, 1, &i, 1, NULL,
                     EDT_PROP_NONE, affinity, &outputEventGuid);
        ocrAddDependence(outputEventGuid, shutdownGuid, i, false);
        ocrAddDependence(dbGuid, eWriteEdtGuid, 0, DB_MODE_EW);
        i++;
    }

    return NULL_GUID;
}
