
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: Create EDT that destroys a DB it did not acquired (edt1: dbRelease/Satisfy edt2:Destroy)
 */

#define TYPE_ELEM_DB int
#define NB_ELEM_DB 20

ocrGuid_t readerEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t dbGuid = depv[0].guid;
    ocrGuid_t evGuid = ((ocrGuid_t*) depv[1].ptr)[0];
    PRINTF("readerEdt: executing, depends on DB guid 0x%lx \n", dbGuid);
    int v = 1;
    int i = 0;
    int * data = (int *) depv[0].ptr;
    while (i < NB_ELEM_DB) {
        ASSERT(data[i] == v++);
        i++;
    }
    PRINTF("readerEdt: release DB guid 0x%lx \n", dbGuid);
    ocrDbRelease(dbGuid);
    PRINTF("readerEdt: satisfy event 0x%lx \n", evGuid);
    ocrEventSatisfy(evGuid, dbGuid);
    return NULL_GUID;
}

ocrGuid_t destroyEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t dbGuid = depv[0].guid;
    PRINTF("destroyEdt: destroying  dbGuid 0x%lx\n", dbGuid);
    ocrDbDestroy(dbGuid);
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Create DB array to store the event guid
    ocrGuid_t * evPtrArray;
    ocrGuid_t evGuidArray;
    ocrDbCreate(&evGuidArray, (void **)&evPtrArray, sizeof(ocrGuid_t), 0, NULL_GUID, NO_ALLOC);

    // Create a DB that contains user data
    ocrGuid_t dbGuid;
    void * dbPtr;
    ocrDbCreate(&dbGuid, &dbPtr, sizeof(TYPE_ELEM_DB) * NB_ELEM_DB, 0, NULL_GUID, NO_ALLOC);
    int v = 1;
    int i = 0;
    int * data = (int *) dbPtr;
    while (i < NB_ELEM_DB) {
        data[i] = v++;
        i++;
    }

    // Create event used by readerEdt to satisfy destroyEdt
    ocrGuid_t eventGuid;
    ocrEventCreate(&eventGuid, OCR_EVENT_STICKY_T, true);
    evPtrArray[0] = eventGuid;
    ocrDbRelease(evGuidArray);

    PRINTF("mainEdt: local DB guid is 0x%lx, dbPtr=%p\n", dbGuid, dbPtr);
    ocrGuid_t destroyEdtTemplateGuid;
    ocrEdtTemplateCreate(&destroyEdtTemplateGuid, destroyEdt, 0, 1);
    ocrGuid_t readerEdtTemplateGuid;
    ocrEdtTemplateCreate(&readerEdtTemplateGuid, readerEdt, 0, 2);

    // Setup destroy EDT depending on the event to be satisfied by readerEdt
    ocrGuid_t destroyEdtGuid;
    ocrEdtCreate(&destroyEdtGuid, destroyEdtTemplateGuid, 0, NULL, 1, NULL,
                 EDT_PROP_NONE, NULL_GUID, NULL);
    ocrAddDependence(eventGuid, destroyEdtGuid, 0, DB_MODE_RO);

    // Setup reader EDT, takes the array of guid as depv
    ocrGuid_t readerEdtGuid;
    ocrEdtCreate(&readerEdtGuid, readerEdtTemplateGuid, 0, NULL, 2, NULL,
                 EDT_PROP_NONE, NULL_GUID, NULL);
    ocrAddDependence(dbGuid, readerEdtGuid, 0, DB_MODE_RO);
    ocrAddDependence(evGuidArray, readerEdtGuid, 1, DB_MODE_RO);

    return NULL_GUID;
}
