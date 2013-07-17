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

/**
 * DESC: Chain an edt to another edt's output event.
 */

// This edt is triggered when the output event of the other edt is satisfied by the runtime
ocrGuid_t chainedEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrShutdown(); // This is the last EDT to execute, terminate
    return NULL_GUID;
}

ocrGuid_t taskForEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // When this edt terminates, the runtime will satisfy its output event automatically
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Current thread is '0' and goes on with user code.
    ocrGuid_t event_guid;
    ocrEventCreate(&event_guid, OCR_EVENT_STICKY_T, true);

    // Setup output event
    ocrGuid_t output_event_guid;
    // Creates the parent EDT
    ocrGuid_t edtGuid;
    ocrGuid_t taskForEdtTemplateGuid;
    ocrEdtTemplateCreate(&taskForEdtTemplateGuid, taskForEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&edtGuid, taskForEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                    /*properties=*/0, NULL_GUID, /*outEvent=*/&output_event_guid);

    // Setup edt input event
    ocrGuid_t input_event_guid;
    ocrEventCreate(&input_event_guid, OCR_EVENT_STICKY_T, true);

    // Create the chained EDT and add input and output events as dependences.
    ocrGuid_t chainedEdtGuid;
    ocrGuid_t chainedEdtTemplateGuid;
    ocrEdtTemplateCreate(&chainedEdtTemplateGuid, chainedEdt, 0 /*paramc*/, 2 /*depc*/);
    ocrEdtCreate(&chainedEdtGuid, chainedEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                    /*properties=*/ EDT_PROP_FINISH, NULL_GUID, /*outEvent=*/NULL);
    ocrAddDependence(output_event_guid, chainedEdtGuid, 0, DB_MODE_RO);
    ocrAddDependence(input_event_guid, chainedEdtGuid, 1, DB_MODE_RO);

    // parent edt: Add dependence, schedule and trigger
    // Note: we don't strictly need to have a dependence here, it's just
    // to get a little bit more control so as to when the root edt gets
    // a chance to be scheduled.
    ocrAddDependence(event_guid, edtGuid, 0, DB_MODE_RO);

    // Transmit the parent edt's guid as a parameter to the chained edt
    // Build input db for the chained edt
    ocrGuid_t * guid_ref;
    ocrGuid_t db_guid;
    ocrDbCreate(&db_guid,(void **) &guid_ref, sizeof(ocrGuid_t), /*flags=*/FLAGS, /*loc=*/NULL_GUID, NO_ALLOC);
    *guid_ref = edtGuid;
    // Satisfy the input slot of the chained edt
    ocrEventSatisfy(input_event_guid, db_guid);

    // Satisfy the parent edt. At this point it should run
    // to completion and satisfy its output event with its guid
    // which should trigger the chained edt since all its input
    // dependencies will be satisfied.
    ocrEventSatisfy(event_guid, NULL_GUID);

    return NULL_GUID;
}
