/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ocr.h"

#define FLAGS 0xdead

ocrGuid_t taskForEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int* res = (int*)depv[0].ptr;
    printf("In the taskForEdt with value %d\n", (*res));
    assert(*res == 42);
    // This is the last EDT to execute, terminate
    ocrShutdown();
    return NULL_GUID;
}

void test () {
    // Current thread is '0' and goes on with user code.
    ocrGuid_t eventGuid;
    ocrEventCreate(&eventGuid, OCR_EVENT_STICKY_T, true);

    // Creates the EDT
    ocrGuid_t edtGuid;
    ocrGuid_t taskForEdtTemplateGuid;
    ocrEdtTemplateCreate(&taskForEdtTemplateGuid, taskForEdt, 0 /*paramc*/, 1 /*depc*/);

    ocrEdtCreate(&edtGuid, taskForEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL,
                 EDT_PARAM_DEF, /*depv=*/NULL, 0, NULL_GUID, NULL);

    // Register a dependence between an event and an edt
    ocrAddDependence(eventGuid, edtGuid, 0, DB_MODE_RO);

    int *k;
    ocrGuid_t db_guid;
    ocrDbCreate(&db_guid,(void **) &k,
            sizeof(int), /*flags=*/FLAGS,
            /*location=*/NULL_GUID,
            NO_ALLOC);
    *k = 42;

    ocrEventSatisfy(eventGuid, db_guid);
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    test();
    //TODO this test needs to be updated if we have a way to
    //call ocr as a library two times in a row.
    //test(argc, argv);

    return NULL_GUID;
}
