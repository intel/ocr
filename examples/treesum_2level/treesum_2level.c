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
    printf("I am summing %d (0x%lx) and %d (0x%lx) and passing along %d (0x%lx)\n",
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

    printf("Got result: %d (0x%lx)\n", *result, (u64)depv[0].guid);

    /* Destroy the input data-block */
    ocrDbDestroy(depv[0].guid);

    /* Last codelet to execute */
    ocrFinish();
    return 0;
}


int main (int argc, char ** argv) {
    ocrEdt_t fctPtrArray[2] = {summer, autumn};
    ocrInit(&argc, argv, 2, fctPtrArray);

    /* Create 4 data-blocks */
    ocrGuid_t dbs[4];
    int *data[4];
    int i;
    for(i = 0; i < 4; ++i) {
        ocrDbCreate(&dbs[i], (void**)&data[i], sizeof(int), /*flags=*/0,
                    /*location=*/NULL, NO_ALLOC);
        *(data[i]) = i;
        printf("Created a data-block with value %d (0x%lx)\n", i, (u64)dbs[i]);
    }

    ocrGuid_t summer1Edt, summer2Edt, summer3Edt, autumnEdt;
    ocrGuid_t autumnEvt, summer1Evt[3], summer2Evt[3], summer3Evt[3];
    ocrGuid_t summer1EvtDbGuid, summer2EvtDbGuid, summer3EvtDbGuid;
    ocrGuid_t *summer1EvtDb, *summer2EvtDb, *summer3EvtDb;

    /* Create final EDT (autumn) */
    ocrEdtCreate(&autumnEdt, autumn, /*paramc=*/0, /*params=*/NULL,
                 /*paramv=*/NULL, /*properties=*/0, /*depc=*/1, /*depv=*/NULL);

    /* Create event */
    ocrEventCreate(&autumnEvt, OCR_EVENT_STICKY_T, true);

    /* Create summers */
    ocrEdtCreate(&summer1Edt, summer, /*paramc=*/0, /*params=*/NULL,
                 /*paramv=*/NULL, /*properties=*/0, /*depc=*/3, /*depv=*/NULL);
    ocrEdtCreate(&summer2Edt, summer, /*paramc=*/0, /*params=*/NULL,
                 /*paramv=*/NULL, /*properties=*/0, /*depc=*/3, /*depv=*/NULL);
    ocrEdtCreate(&summer3Edt, summer, /*paramc=*/0, /*params=*/NULL,
                 /*paramv=*/NULL, /*properties=*/0, /*depc=*/3, /*depv=*/NULL);

    /* Create events for summers */
    for(i = 0; i < 3; ++i) {
        ocrEventCreate(&summer1Evt[i], OCR_EVENT_STICKY_T, true);
    }
    for(i = 0; i < 3; ++i) {
        ocrEventCreate(&summer2Evt[i], OCR_EVENT_STICKY_T, true);
    }
    for(i = 0; i < 3; ++i) {
        ocrEventCreate(&summer3Evt[i], OCR_EVENT_STICKY_T, true);
    }

    /* Create data-blocks containing events */
    ocrDbCreate(&summer1EvtDbGuid, (void**)&summer1EvtDb, sizeof(ocrGuid_t), /*flags=*/0,
                /*location=*/NULL, NO_ALLOC);
    *summer1EvtDb = summer3Evt[0];
    ocrDbCreate(&summer2EvtDbGuid, (void**)&summer2EvtDb, sizeof(ocrGuid_t), /*flags=*/0,
                /*location=*/NULL, NO_ALLOC);
    *summer2EvtDb = summer3Evt[1];
    ocrDbCreate(&summer3EvtDbGuid, (void**)&summer3EvtDb, sizeof(ocrGuid_t), /*flags=*/0,
                /*location=*/NULL, NO_ALLOC);
    *summer3EvtDb = autumnEvt;

    /* Link up dependences */
    for(i = 0; i < 3; ++i) {
        ocrAddDependence(summer1Evt[i], summer1Edt, i);
    }
    for(i = 0; i < 3; ++i) {
        ocrAddDependence(summer2Evt[i], summer2Edt, i);
    }
    for(i = 0; i < 3; ++i) {
        ocrAddDependence(summer3Evt[i], summer3Edt, i);
    }

    ocrAddDependence(autumnEvt, autumnEdt, 0);

    /* "Schedule" EDTs */
    ocrEdtSchedule(autumnEdt);
    ocrEdtSchedule(summer1Edt);
    ocrEdtSchedule(summer2Edt);
    ocrEdtSchedule(summer3Edt);

    printf("Done all scheduling, now going to satisfy\n");

    /* Satisfy dependences passing data */
    ocrEventSatisfy(summer1Evt[0], dbs[0]);
    ocrEventSatisfy(summer1Evt[1], dbs[1]);
    ocrEventSatisfy(summer1Evt[2], summer1EvtDbGuid);

    ocrEventSatisfy(summer2Evt[0], dbs[2]);
    ocrEventSatisfy(summer2Evt[1], dbs[3]);
    ocrEventSatisfy(summer2Evt[2], summer2EvtDbGuid);

    ocrEventSatisfy(summer3Evt[2], summer3EvtDbGuid);

    /* Finalize */
    ocrCleanup();
    return 0;
}
