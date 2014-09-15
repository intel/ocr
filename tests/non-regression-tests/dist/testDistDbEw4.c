
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - Competing writer EDTs with multiple EW dependences (in-order deps, no early DB Release)
 */

#define TYPE_ELEM_DB int

// BIG
#define NB_DBS 20
#define NB_DBS_PER_EDT 5
#define NB_ELEM_DB 20
#define NB_WRITERS 100

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

    // Create DB @ affinity
    ocrGuid_t dbGuid[NB_DBS];
    u64 db = 0;
    while (db < NB_DBS) {
        void * dbPtr;
        u32 nbElem = NB_ELEM_DB;
        ocrDbCreate(&dbGuid[db], &dbPtr, sizeof(TYPE_ELEM_DB) * NB_ELEM_DB, 0, affinities[db%affinityCount], NO_ALLOC);
        u64 i = 0;
        int * data = (int *) dbPtr;
        while (i < nbElem) {
            data[i] = -1;
            i++;
        }
        ocrDbRelease(dbGuid[db]);
        db++;
    }

    ocrGuid_t shutdownEdtTemplateGuid;
    ocrEdtTemplateCreate(&shutdownEdtTemplateGuid, shutdownEdt, 0, NB_WRITERS);
    ocrGuid_t shutdownGuid;
    ocrEdtCreate(&shutdownGuid, shutdownEdtTemplateGuid, 0, NULL, NB_WRITERS, NULL,
                 EDT_PROP_NONE, NULL_GUID, NULL);

    // create writer EDT for the DB in EW mode @ affinity
    ocrGuid_t eWriteEdtTemplateGuid;
    ocrEdtTemplateCreate(&eWriteEdtTemplateGuid, eWriteEdt, 1, NB_DBS_PER_EDT);

    u64 i = 0;
    while (i < NB_WRITERS) {
        ocrGuid_t outputEventGuid;
        ocrGuid_t eWriteEdtGuid;
        ocrEdtCreate(&eWriteEdtGuid, eWriteEdtTemplateGuid, 1, &i, NB_DBS_PER_EDT, NULL,
                     EDT_PROP_NONE, affinities[i%affinityCount], &outputEventGuid);
        ocrAddDependence(outputEventGuid, shutdownGuid, i, false);
        u32 j = 0;
        while (j < NB_DBS_PER_EDT) {
            ocrAddDependence(dbGuid[(j+i)%NB_DBS], eWriteEdtGuid, j, DB_MODE_EW);
            j++;
        }
        i++;
    }

    return NULL_GUID;
}
