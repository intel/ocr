
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */




#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - EDT operates on remote (big) DB then satisfy EDT at DB's node and check write-back happened
 */

#define TYPE_ELEM_DB int
#define NB_ELEM_DB 262144 //(1MB)

ocrGuid_t addEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t dbCloneGuid = (ocrGuid_t) depv[0].guid;
    PRINTF("[remote] addEdt: executing, depends on remote DB guid 0x%lx \n", dbCloneGuid);
    TYPE_ELEM_DB v = 1;
    int i = 0;
    TYPE_ELEM_DB * data = (TYPE_ELEM_DB *) depv[0].ptr;
    while (i < NB_ELEM_DB) {
        ASSERT (data[i] == v++);
        data[i] += 100;
        i++;
    }
    PRINTF("[remote] addEdt: DB written to, releasing...\n");
    ocrDbRelease(dbCloneGuid); // Forces writeback
    ASSERT(paramc == 1);
    ocrGuid_t eventGuid = (ocrGuid_t) paramv[0];
    PRINTF("[remote] addEdt: Satisfy checkerEdt's event guid 0x%lx\n", eventGuid);
    ocrEventSatisfy(eventGuid, dbCloneGuid);
    return NULL_GUID;
}

ocrGuid_t checkerEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t dbCloneGuid = (ocrGuid_t) depv[0].guid;
    PRINTF("[local] CheckerEdt: executing, depends on remote DB guid 0x%lx \n", dbCloneGuid);
    TYPE_ELEM_DB v = 101;
    int i = 0;
    TYPE_ELEM_DB * data = (TYPE_ELEM_DB *) depv[0].ptr;
    while (i < NB_ELEM_DB) {
        ASSERT (data[i] == v++);
        i++;
    }
    PRINTF("[local] CheckerEdt: DB values checked, updated correctly\n");
    //TODO: don't need to do that but we don't support RO mode right now and
    //the write back done after the EDT ends may race with shutdown
    ocrDbRelease(dbCloneGuid);
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ASSERT(affinityCount >= 1);
    ocrGuid_t affinities[affinityCount];
    ocrAffinityGet(AFFINITY_PD, &affinityCount, affinities);
    ocrGuid_t edtAffinity = affinities[affinityCount-1]; //TODO this implies we know current PD is '0'

    // Create a DB
    void * dbPtr;
    ocrGuid_t dbGuid;
    u64 nbElem = NB_ELEM_DB;
    ocrDbCreate(&dbGuid, &dbPtr, sizeof(TYPE_ELEM_DB) * NB_ELEM_DB, 0, NULL_GUID, NO_ALLOC);
    int v = 1;
    int i = 0;
    int * data = (int *) dbPtr;
    while (i < nbElem) {
        data[i] = v++;
        i++;
    }
    ocrDbRelease(dbGuid);
    PRINTF("[local] mainEdt: DB guid is 0x%lx, dbPtr=%p\n",dbGuid, dbPtr);

    ocrGuid_t addEdtTemplateGuid;
    ocrEdtTemplateCreate(&addEdtTemplateGuid, addEdt, 1, 1);
    ocrGuid_t checkerEdtTemplateGuid;
    ocrEdtTemplateCreate(&checkerEdtTemplateGuid, checkerEdt, 0, 1);

    // Create local event
    ocrGuid_t eventGuid;
    ocrEventCreate(&eventGuid, OCR_EVENT_STICKY_T, true);
    PRINTF("[local] mainEdt: Creating event with guid 0x%lx\n", eventGuid);
    // Create local EDT depending on event being satisfied
    ocrGuid_t checkerEdtGuid;
    ocrEdtCreate(&checkerEdtGuid, checkerEdtTemplateGuid, 0, NULL, 1, &eventGuid,
                 EDT_PROP_NONE, NULL_GUID, NULL);

    // create remote edt that depends the db which is automatically cloned
    u64 rparamv = (u64) eventGuid; // NASTY cast: the event to satisfy later on
    ocrGuid_t addEdtGuid;
    ocrEdtCreate(&addEdtGuid, addEdtTemplateGuid, 1, &rparamv, 1, &dbGuid,
                 EDT_PROP_NONE, edtAffinity, NULL);

    return NULL_GUID;
}
