/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: This program spawns a "latch" event, N task "A"'s and N "once" events.
 * Task A[i] satisfies event[i], which (eventually) allows the latch counter
 * to drop to zero, which runs done() and passes the test.
 * [references issue #197]
 */

// Uncomment usleep to help reproduce the issue
// #include <unistd.h>

#define N 1000

ocrGuid_t done(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("Success!\n");
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t A(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int i = paramv[0];
    ocrGuid_t *event_array = (ocrGuid_t*)depv[0].ptr;
    if(!event_array || (((u64)event_array) == ((u64)-3)))
        PRINTF("[%d] I AM GOING TO CRASH: depv[0].guid=%ld, depv[0].ptr=%p\n", i, depv[0].guid, depv[0].ptr);
    ocrEventSatisfy(event_array[i], NULL_GUID);
    // usleep(300);
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {
    u64 i;
    ocrGuid_t event_array_guid, *event_array, latch_guid, A_template, done_template, done_edt;

    ocrDbCreate(&event_array_guid, (void**)&event_array, N*sizeof(ocrGuid_t), DB_PROP_NONE, NULL_GUID, NO_ALLOC);
    ocrEdtTemplateCreate(&A_template, A, 1, 1);
    ocrEdtTemplateCreate(&done_template, done, 0, 1);
    ocrEventCreate(&latch_guid, OCR_EVENT_LATCH_T, FALSE);
    ocrAddDependence(NULL_GUID, latch_guid, OCR_EVENT_LATCH_INCR_SLOT, DB_MODE_RO);
    ocrEdtCreate(&done_edt, done_template, 0, NULL, 1, &latch_guid, EDT_PROP_NONE, NULL_GUID, NULL);

    for(i = 0; i < N; i++) {
        ocrEventCreate(&event_array[i], OCR_EVENT_ONCE_T, FALSE);
    }

    for(i = 0; i < N; i++) {
        ocrGuid_t A_edt;
        ocrAddDependence(NULL_GUID     , latch_guid, OCR_EVENT_LATCH_INCR_SLOT, DB_MODE_RO);
        ocrAddDependence(event_array[i], latch_guid, OCR_EVENT_LATCH_DECR_SLOT, DB_MODE_RO);
        ocrEdtCreate(&A_edt, A_template,
                     1, &i,
                     1, &event_array_guid,
                     EDT_PROP_NONE, NULL_GUID, NULL);
    }

    ocrAddDependence(NULL_GUID, latch_guid, OCR_EVENT_LATCH_DECR_SLOT, DB_MODE_RO);

    return NULL_GUID;
}
