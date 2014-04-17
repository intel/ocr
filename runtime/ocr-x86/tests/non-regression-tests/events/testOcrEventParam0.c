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
 * DESC: Test passing paramv to ocrEdtCreate
 */

#define FLAGS 0xdead

ocrGuid_t taskForEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    printf("In the taskForEdt with value %d\n", (int) paramv[0]);
    assert(paramc == 1);
    assert(paramv[0] == 32);
    assert(*((u64*)depv[0].ptr) == 42);
    // This is the last EDT to execute, terminate
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Current thread is '0' and goes on with user code.
    ocrGuid_t eventGuid;
    ocrEventCreate(&eventGuid, OCR_EVENT_STICKY_T, true);

    // Creates the EDT
    u32 nparamc = 1;
    u64 * nparamv = (u64 *) malloc(sizeof(u64));
    nparamv[0] = 32;

    ocrGuid_t edtGuid;
    ocrGuid_t taskForEdtTemplateGuid;
    ocrEdtTemplateCreate(&taskForEdtTemplateGuid, taskForEdt, nparamc, 1 /*depc*/);
    ocrEdtCreate(&edtGuid, taskForEdtTemplateGuid, EDT_PARAM_DEF, nparamv, EDT_PARAM_DEF, /*depv=*/NULL,
                    /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

    // Register a dependence between an event and an edt
    ocrAddDependence(eventGuid, edtGuid, 0, DB_MODE_RO);

    u64 *k;
    ocrGuid_t dbGuid;
    ocrDbCreate(&dbGuid,(void **) &k,
            sizeof(u64), /*flags=*/FLAGS,
            /*location=*/NULL_GUID,
            NO_ALLOC);
    *k = 42;

    ocrEventSatisfy(eventGuid, dbGuid);

    return NULL_GUID;
}
