
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: Create EDT that destroys a DB it did not acquired
 */

#define TYPE_ELEM_DB int
#define NB_ELEM_DB 20

ocrGuid_t destroyEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t * dbGuidArray = (ocrGuid_t *) depv[0].ptr;
    PRINTF("destroyEdt: destroy DB guid 0x%lx \n", dbGuidArray[0]);
    ocrDbDestroy(dbGuidArray[0]);
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Create a DB
    ocrGuid_t * dbPtrArray;
    ocrGuid_t dbGuidArray;
    ocrDbCreate(&dbGuidArray, (void **)&dbPtrArray, sizeof(ocrGuid_t), 0, NULL_GUID, NO_ALLOC);
    // Set the other db guid in the guid array
    void * dbPtr;
    ocrDbCreate(&dbPtrArray[0], &dbPtr, sizeof(TYPE_ELEM_DB) * NB_ELEM_DB, 0, NULL_GUID, NO_ALLOC);
    PRINTF("mainEdt: local DB guid is 0x%lx\n", dbPtrArray[0]);
    ocrDbRelease(dbGuidArray);
    // create local edt that depends on the remote edt, the db is automatically cloned
    ocrGuid_t destroyEdtTemplateGuid;
    ocrEdtTemplateCreate(&destroyEdtTemplateGuid, destroyEdt, 0, 1);

    ocrGuid_t destroyEdtGuid;
    ocrEdtCreate(&destroyEdtGuid, destroyEdtTemplateGuid, 0, NULL, 1, NULL,
                 EDT_PROP_NONE, NULL_GUID, NULL);
    // Intentionally did not release the DB @ dbPtrArray[0]
    ocrAddDependence(dbGuidArray, destroyEdtGuid, 0, DB_MODE_RO);
    return NULL_GUID;
}
