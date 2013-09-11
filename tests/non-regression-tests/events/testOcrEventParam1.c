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
 * DESC: Test passing paramv and depv to ocrEdtCreate
 */

#define FLAGS 0xdead

ocrGuid_t taskForEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    printf("In the taskForEdt with value %d\n", (int)paramv[0]);
    assert(paramc == 1);
    assert(paramv[0] == 32);
    void * ptr = depv[0].ptr;
    assert(*((int*)ptr) == 42);
    // This is the last EDT to execute, terminate
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Current thread is '0' and goes on with user code.
    ocrGuid_t ndepv[1];
    ocrGuid_t eventGuid;
    ocrEventCreate(&eventGuid, OCR_EVENT_STICKY_T, true);
    ndepv[0] = eventGuid;

    // Creates the EDT
    u32 nparamc = 1;
    u64 * nparamv = (u64 *) malloc(sizeof(u64));
    nparamv[0] = 32;

    // 'ndepv' stores dependencies, so no need to call
    // ocrAddDependence later on to register events.
    ocrGuid_t edtGuid;
    ocrGuid_t taskForEdtTemplateGuid;
    ocrEdtTemplateCreate(&taskForEdtTemplateGuid, taskForEdt, nparamc, 1 /*depc*/);
    ocrEdtCreate(&edtGuid, taskForEdtTemplateGuid, EDT_PARAM_DEF, nparamv, EDT_PARAM_DEF, /*depv=*/ndepv,
                    /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

    int *k;
    ocrGuid_t dbGuid;
    ocrDbCreate(&dbGuid,(void **) &k,
            sizeof(int), /*flags=*/FLAGS,
            /*location=*/NULL_GUID,
            NO_ALLOC);
    *k = 42;

    ocrEventSatisfy(eventGuid, dbGuid);

    return NULL_GUID;
}
