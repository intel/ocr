/**
 * @brief Simple "hello-world" example for OCR showing a parallel
 * sum of 4 elements.
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include <stdio.h>
#include <stdlib.h>

#include "ocr.h"

/* Do the addition */
ocrGuid_t summer(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int *result;
    ocrGuid_t resultGuid;

    /* Get both numbers */
    int *n1 = (int*)depv[0].ptr, *n2 = (int*)depv[1].ptr;

    /* Get event to satisfy */
    ocrGuid_t *evt = (ocrGuid_t*)depv[2].ptr;

    /* Create data-block to put result */
    ocrDbCreate(&resultGuid, (void**)&result, sizeof(int), /*flags=*/0, /*location=*/NULL_GUID, NO_ALLOC);
    *result = *n1 + *n2;

    /* Say hello */
    printf("I am summing %d (GUID: 0x%lx) and %d (GUID: 0x%lx) and passing along %d (GUID: 0x%lx)\n",
           *n1, (u64)depv[0].guid, *n2, (u64)depv[1].guid,
           *result, (u64)resultGuid);

    /* Satisfy whomever is waiting on me */
    ocrEventSatisfy(*evt, resultGuid);

    return NULL_GUID;
}

/* Print the result */
ocrGuid_t autumn(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int * result = (int*)depv[0].ptr;

    printf("Got result: %d (GUID: 0x%lx)\n", *result, (u64)depv[0].guid);

    /* Last EDT to execute */
    ocrShutdown();
    return NULL_GUID;
}


ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    //TODO unpack argc/argv

    if(argc != 3) {
        printf("Usage %s <num1> <num2>\n", argv[0]);
        return -1;
    }

    /* Create 2 data-blocks */
    ocrGuid_t dbs[2];
    int *data[2];
    int i;
    for(i = 0; i < 2; ++i) {
        ocrDbCreate(&dbs[i], (void**)&data[i], sizeof(int), /*flags=*/0,
                    /*location=*/NULL_GUID, NO_ALLOC);
        *(data[i]) = atoi(argv[i+1]);
        printf("Created a data-block with value %d (GUID: 0x%lx)\n", i, (u64)dbs[i]);
    }

    ocrGuid_t summerEdt, autumnEdt;
    ocrGuid_t autumnEvt, summerEvt[3];
    ocrGuid_t summerEvtDbGuid;
    ocrGuid_t *summerEvtDb;

    /* Create final EDT (autumn) */
    ocrGuid_t autumnEdtTemplateGuid;
    ocrEdtTemplateCreate(&autumnEdtTemplateGuid, autumn, 0/*paramc*/, 1/*depc*/);
    ocrEdtCreate(&autumnEdt, autumnEdtTemplateGuid, /*paramc=*/EDT_PARAM_DEF,
                 /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

    /* Create event */
    ocrEventCreate(&autumnEvt, OCR_EVENT_STICKY_T, true);

    /* Create summer */
    ocrGuid_t summerEdtTemplateGuid;
    ocrEdtTemplateCreate(&summerEdtTemplateGuid, summer, 0/*paramc*/, 3/*depc*/);
    ocrEdtCreate(&summerEdt, summerEdtTemplateGuid, /*paramc=*/EDT_PARAM_DEF,
                 /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

    /* Create events for summer */
    for(i = 0; i < 3; ++i) {
        ocrEventCreate(&summerEvt[i], OCR_EVENT_STICKY_T, true);
    }

    /* Create data-block containing event */
    ocrDbCreate(&summerEvtDbGuid, (void**)&summerEvtDb, sizeof(ocrGuid_t), /*flags=*/0,
                /*location=*/NULL_GUID, NO_ALLOC);
    *summerEvtDb = autumnEvt;

    /* Link up dependencees */
    for(i = 0; i < 3; ++i) {
        ocrAddDependence(summerEvt[i], summerEdt, i, DB_MODE_RO);
    }

    ocrAddDependence(autumnEvt, autumnEdt, 0, DB_MODE_RO);

    printf("Done all scheduling, now going to satisfy\n");

    /* Satisfy dependences passing data */
    ocrEventSatisfy(summerEvt[0], dbs[0]);
    ocrEventSatisfy(summerEvt[1], dbs[1]);
    ocrEventSatisfy(summerEvt[2], summerEvtDbGuid);

    return 0;
}
