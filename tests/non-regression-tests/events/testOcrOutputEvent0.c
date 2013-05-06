/* Copyright (c) 2012, Rice University

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Intel Corporation
     nor the names of its contributors may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
ocrGuid_t chained_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    ocrFinish(); // This is the last EDT to execute, terminate
    return NULL_GUID;
}

ocrGuid_t task_for_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    // When this edt terminates, the runtime will satisfy its output event automatically
    return NULL_GUID;
}

int main (int argc, char ** argv) {
    ocrEdt_t fctPtrArray [1];
    fctPtrArray[0] = &task_for_edt;
    ocrInit(&argc, argv, 1, fctPtrArray);

    // Current thread is '0' and goes on with user code.
    ocrGuid_t event_guid;
    ocrEventCreate(&event_guid, OCR_EVENT_STICKY_T, true);

    // Setup output event
    ocrGuid_t output_event_guid;
    ocrEventCreate(&output_event_guid, OCR_EVENT_STICKY_T, true);
    // Setup edt input event
    ocrGuid_t input_event_guid;
    ocrEventCreate(&input_event_guid, OCR_EVENT_STICKY_T, true);

    // Create the chained EDT and add input and output events as dependences.
    ocrGuid_t chained_edt_guid;
    ocrEdtCreate(&chained_edt_guid, chained_edt, /*paramc=*/0, /*params=*/ NULL,
            /*paramv=*/NULL, /*properties=*/0, /*depc=*/2, /*depv=*/NULL, NULL_GUID);
    ocrAddDependence(output_event_guid, chained_edt_guid, 0);
    ocrAddDependence(input_event_guid, chained_edt_guid, 1);
    ocrEdtSchedule(chained_edt_guid);

    // Creates the parent EDT, add dependence, schedule and trigger
    // Note: we don't strictly need to have a dependence here, it's just
    // to get a little bit more control so as to when the root edt gets
    // a chance to be scheduled.
    ocrGuid_t edt_guid;
    ocrEdtCreate(&edt_guid, task_for_edt, /*paramc=*/0, /*params=*/ NULL,
            /*paramv=*/NULL, /*properties=*/0, /*depc=*/1, /*depv=*/NULL, output_event_guid);
    ocrAddDependence(event_guid, edt_guid, 0);
    ocrEdtSchedule(edt_guid);

    // Transmit the parent edt's guid as a parameter to the chained edt
    // Build input db for the chained edt
    ocrGuid_t * guid_ref;
    ocrGuid_t db_guid;
    ocrDbCreate(&db_guid,(void **) &guid_ref, sizeof(ocrGuid_t), /*flags=*/FLAGS, /*loc=*/NULL, NO_ALLOC);
    *guid_ref = edt_guid;
    // Satisfy the input slot of the chained edt
    ocrEventSatisfy(input_event_guid, db_guid);

    // Satisfy the parent edt. At this point it should run
    // to completion and satisfy its output event with its guid
    // which should trigger the chained edt since all its input
    // dependencies will be satisfied.
    ocrEventSatisfy(event_guid, NULL_GUID);

    ocrCleanup();

    return 0;
}
