/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>
#include <assert.h>

#include "ocr.h"

/**
 * DESC: Test using a future Event GUID as an EDT return value
 */

#define N 8

ocrGuid_t consumer(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t *depv) {
    int i, *ptr = (int*)depv[0].ptr;
    for(i = 0; i < N; i++) assert(N-i == ptr[i]);
    printf("Everything went OK\n");
    ocrShutdown();
    return NULL_GUID;
}
ocrGuid_t producer2(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t *depv) {
    ocrGuid_t db_guid;
    int i, *ptr;
    ocrDbCreate(&db_guid, (void **)&ptr, sizeof(*ptr)*N,
                /*flags=*/DB_PROP_NONE, /*location=*/NULL_GUID, NO_ALLOC);
    for(i = 0; i < N; i++) ptr[i] = N - i;
    return db_guid;
}
ocrGuid_t producer1(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t *depv) {
    ocrGuid_t producer_template, producer_edt, producer_done_event;
    ocrEdtTemplateCreate(&producer_template, producer2, 0, 0);
    ocrEdtCreate(&producer_edt, producer_template, 0, NULL, 0, NULL,
                 EDT_PROP_NONE, NULL_GUID, &producer_done_event);
    return producer_done_event;
}
ocrGuid_t mainEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t *depv) {
    ocrGuid_t producer_template, consumer_template;
    ocrGuid_t producer_edt, consumer_edt;
    ocrGuid_t producer_done_event;
    ocrEdtTemplateCreate(&producer_template, producer1, 0, 0);
    ocrEdtTemplateCreate(&consumer_template, consumer , 0, 1);
    ocrEdtCreate(&producer_edt, producer_template, 0, NULL, 0, NULL,
                 EDT_PROP_NONE, NULL_GUID, &producer_done_event);
    ocrEdtCreate(&consumer_edt, consumer_template, 0, NULL, 1, NULL,
                 EDT_PROP_NONE, NULL_GUID, NULL);
    /* create consumer dependency on producer output */
    ocrAddDependence(producer_done_event,consumer_edt,0,DB_MODE_RO);
    return NULL_GUID;
}
