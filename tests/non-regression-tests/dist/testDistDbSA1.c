
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */




#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - create DB/SA with remote affinity, write, create remote edt checker
 */

#define TYPE_ELEM_DB int
#define NB_ELEM_DB 20

ocrGuid_t remoteEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t dbCloneGuid = (ocrGuid_t) depv[0].guid;
    PRINTF("[remote] RemoteEdt: executing, depends on local DB guid 0x%lx \n", dbCloneGuid);
    TYPE_ELEM_DB v = 1;
    int i = 0;
    TYPE_ELEM_DB * data = (TYPE_ELEM_DB *) depv[0].ptr;
    while (i < NB_ELEM_DB) {
        ASSERT (data[i] == v++);
        i++;
    }
    PRINTF("[remote] RemoteEdt: DB/SA checked\n");
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ASSERT(affinityCount >= 1);
    ocrGuid_t affinities[affinityCount];
    ocrAffinityGet(AFFINITY_PD, &affinityCount, affinities);
    ocrGuid_t otherAffinity = affinities[affinityCount-1]; //TODO this implies we know current PD is '0'

    // Create a DB
    void * dbPtr;
    ocrGuid_t dbGuid;
    u64 nbElem = NB_ELEM_DB;
    ocrDbCreate(&dbGuid, &dbPtr, sizeof(TYPE_ELEM_DB) * NB_ELEM_DB, DB_PROP_SINGLE_ASSIGNMENT, otherAffinity, NO_ALLOC);
    PRINTF("[local] mainEdt: create remote DB/SA guid is 0x%lx, dbPtr=%p\n",dbGuid, dbPtr);
    int v = 1;
    int i = 0;
    int * data = (int *) dbPtr;
    while (i < nbElem) {
        data[i] = v++;
        i++;
    }
    // Triggers the write-back
    PRINTF("[local] mainEdt: release remote DB/SA guid is 0x%lx, dbPtr=%p\n",dbGuid, dbPtr);
    ocrDbRelease(dbGuid);

    // create local edt that depends on the remote edt, the db is automatically cloned
    ocrGuid_t remoteEdtTemplateGuid;
    ocrEdtTemplateCreate(&remoteEdtTemplateGuid, remoteEdt, 0, 1);

    PRINTF("[local] mainEdt: create remote EDT to check the DB/SA has been written back\n");
    ocrGuid_t remoteEdtGuid;
    ocrEdtCreate(&remoteEdtGuid, remoteEdtTemplateGuid, 0, NULL, 1, &dbGuid,
                 EDT_PROP_NONE, otherAffinity, NULL);

    return NULL_GUID;
}
