/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ocr.h"

/**
 * DESC: Mix sticky and once event in depv, satisfy in reverse order
 */

ocrGuid_t taskForEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT(((u32*)depv[0].ptr)[0] == 11);
    ASSERT(((u32*)depv[1].ptr)[0] == 22);
    ASSERT(((u32*)depv[2].ptr)[0] == 33);
    ASSERT(((u32*)depv[3].ptr)[0] == 44);
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t once0;
    ocrEventCreate(&once0, OCR_EVENT_ONCE_T, true);
    ocrGuid_t once1;
    ocrEventCreate(&once1, OCR_EVENT_ONCE_T, true);
    ocrGuid_t sticky0;
    ocrEventCreate(&sticky0, OCR_EVENT_STICKY_T, true);
    ocrGuid_t sticky1;
    ocrEventCreate(&sticky1, OCR_EVENT_STICKY_T, true);

    u32 *db0Ptr;
    ocrGuid_t db0Guid;
    ocrDbCreate(&db0Guid,(void **)&db0Ptr, sizeof(u32), 0, NULL_GUID, NO_ALLOC);
    db0Ptr[0] = 11;
    ocrDbRelease(db0Guid);
    u32 *db1Ptr;
    ocrGuid_t db1Guid;
    ocrDbCreate(&db1Guid,(void **)&db1Ptr, sizeof(u32), 0, NULL_GUID, NO_ALLOC);
    db1Ptr[0] = 22;
    ocrDbRelease(db1Guid);
    u32 *db2Ptr;
    ocrGuid_t db2Guid;
    ocrDbCreate(&db2Guid,(void **)&db2Ptr, sizeof(u32), 0, NULL_GUID, NO_ALLOC);
    db2Ptr[0] = 33;
    ocrDbRelease(db2Guid);
    u32 *db3Ptr;
    ocrGuid_t db3Guid;
    ocrDbCreate(&db3Guid,(void **)&db3Ptr, sizeof(u32), 0, NULL_GUID, NO_ALLOC);
    db3Ptr[0] = 44;
    ocrDbRelease(db3Guid);

    // Creates the EDT
    ocrGuid_t edtGuid;
    ocrGuid_t taskForEdtTemplateGuid;
    ocrEdtTemplateCreate(&taskForEdtTemplateGuid, taskForEdt, 0 /*paramc*/, 4 /*depc*/);
    ocrEdtCreate(&edtGuid, taskForEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

    // Register a dependence between an event and an edt
    ocrAddDependence(sticky0, edtGuid, 0, DB_MODE_RO);
    ocrAddDependence(once0, edtGuid, 1, DB_MODE_RO);
    ocrAddDependence(sticky1, edtGuid, 2, DB_MODE_RO);
    ocrAddDependence(once1, edtGuid, 3, DB_MODE_RO);

    ocrEventSatisfy(once1, db3Guid);
    ocrEventSatisfy(sticky1, db2Guid);
    ocrEventSatisfy(once0, db1Guid);
    ocrEventSatisfy(sticky0, db0Guid);
    return NULL_GUID;
}
