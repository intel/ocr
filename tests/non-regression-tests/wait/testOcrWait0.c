/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */





#include "ocr.h"

// Only tested when ocr-lib interface is available
#ifdef OCR_LIBRARY_ITF

#include "extensions/ocr-runtime-itf.h"

#define FLAGS 0xdead
#define ELS_OFFSET 0

void someUserFunction() {
    // ocrGuid_t els_data_guid = ocrElsGet(ELS_OFFSET);
    // void * datum;
    // ocrDbAcquire(els_data_guid, &datum, 0);
    // ASSERT(*((int *)datum) == 42);
    // ocrDbRelease(els_data_guid);
}

ocrGuid_t taskForEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // ocrGuid_t data = depv[0].guid;
    // ocrElsSet(ELS_OFFSET, data);
    // someUserFunction();
    // This is the last EDT to execute, terminate
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Current thread is '0' and goes on with user code.
    ocrGuid_t eventGuid;
    ocrEventCreate(&eventGuid, OCR_EVENT_STICKY_T, true);

    // Creates the EDT
    ocrGuid_t edtGuid;
    ocrGuid_t taskForEdtTemplateGuid;
    ocrEdtTemplateCreate(&taskForEdtTemplateGuid, taskForEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&edtGuid, taskForEdtTemplateGuid, EDT_PARAM_DEF, NULL, EDT_PARAM_DEF, NULL, 0, NULL_GUID, NULL);
    // Register a dependence between an event and an edt
    ocrAddDependence(eventGuid, edtGuid, 0);

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

#else

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrShutdown();
    return NULL_GUID;
}

#endif
