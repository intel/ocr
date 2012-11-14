/**
 * @brief Simple "hello-world" example for OCR showing a parallel
 * sum of 4 elements.
 *
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-11-12
 *
 * Copyright (c) 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>

#include "ocr.h"

/* Do the addition */
u8 summer(u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int *result;
    ocrGuid_t resultGuid;

    /* Get both numbers */
    int *n1 = (int*)depv[0].ptr, *n2 = (int*)depv[1].ptr;

    /* Get event to satisfy */
    ocrGuid_t *evt = (ocrGuid_t*)depv[2].ptr;

    /* Create data-block to put result */
    ocrDbCreate(&resultGuid, (void**)&result, sizeof(int), /*flags=*/0, /*location=*/NULL, NO_ALLOC);
    *result = *n1 + *n2;

    /* Say hello */
    printf("I am summing %d (GUID: 0x%lx) and %d (GUID: 0x%lx) and passing along %d (GUID: 0x%lx)\n",
           *n1, (u64)depv[0].guid, *n2, (u64)depv[1].guid,
           *result, (u64)resultGuid);

    /* Satisfy whomever is waiting on me */
    ocrEventSatisfy(*evt, resultGuid);

    /* Free inputs */
    ocrDbDestroy(depv[0].guid);
    ocrDbDestroy(depv[1].guid);
    ocrDbDestroy(depv[2].guid);
    return 0;
}

/* Print the result */
u8 autumn(u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int * result = (int*)depv[0].ptr;

    printf("Got result: %d (GUID: 0x%lx)\n", *result, (u64)depv[0].guid);

    /* Destroy the input data-block */
    ocrDbDestroy(depv[0].guid);

    /* Last EDT to execute */
    ocrFinish();
    return 0;
}


int main (int argc, char ** argv) {
    ocrEdt_t fctPtrArray[2] = {summer, autumn};
    ocrInit(&argc, argv, 2, fctPtrArray); /* No machine description */

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
                    /*location=*/NULL, NO_ALLOC);
        *(data[i]) = atoi(argv[i+1]);
        printf("Created a data-block with value %d (GUID: 0x%lx)\n", i, (u64)dbs[i]);
    }

    ocrGuid_t summerEdt, autumnEdt;
    ocrGuid_t autumnEvt, summerEvt[3];
    ocrGuid_t summerEvtDbGuid;
    ocrGuid_t *summerEvtDb;

    /* Create final EDT (autumn) */
    ocrEdtCreate(&autumnEdt, autumn, /*paramc=*/0, /*params=*/NULL,
                 /*paramv=*/NULL, /*properties=*/0, /*depc=*/1, /*depv=*/NULL);

    /* Create event */
    ocrEventCreate(&autumnEvt, OCR_EVENT_STICKY_T, true);

    /* Create summer */
    ocrEdtCreate(&summerEdt, summer, /*paramc=*/0, /*params=*/NULL,
                 /*paramv=*/NULL, /*properties=*/0, /*depc=*/3, /*depv=*/NULL);

    /* Create events for summer */
    for(i = 0; i < 3; ++i) {
        ocrEventCreate(&summerEvt[i], OCR_EVENT_STICKY_T, true);
    }

    /* Create data-block containing event */
    ocrDbCreate(&summerEvtDbGuid, (void**)&summerEvtDb, sizeof(ocrGuid_t), /*flags=*/0,
                /*location=*/NULL, NO_ALLOC);
    *summerEvtDb = autumnEvt;

    /* Link up dependencies */
    for(i = 0; i < 3; ++i) {
        ocrAddDependency(summerEvt[i], summerEdt, i);
    }

    ocrAddDependency(autumnEvt, autumnEdt, 0);

    /* "Schedule" EDTs (order does not matter) */
    ocrEdtSchedule(autumnEdt);
    ocrEdtSchedule(summerEdt);

    printf("Done all scheduling, now going to satisfy\n");

    /* Satisfy dependencies passing data */
    ocrEventSatisfy(summerEvt[0], dbs[0]);
    ocrEventSatisfy(summerEvt[1], dbs[1]);
    ocrEventSatisfy(summerEvt[2], summerEvtDbGuid);

    /* Finalize */
    ocrCleanup();
    return 0;
}
