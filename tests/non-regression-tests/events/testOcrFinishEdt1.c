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
#define N 100
/**
 * DESC:
 */

// This edt is triggered when the output event of the other edt is satisfied by the runtime
ocrGuid_t terminate_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    printf("Terminate\n");
    ocrFinish(); // This is the last EDT to execute, terminate
    return NULL_GUID;
}

ocrGuid_t main_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    printf("Running Main\n");
    ocrFinish();
    return NULL_GUID;
}

int main (int argc, char ** argv) {
    ocrEdt_t fctPtrArray [2];
    fctPtrArray[0] = &main_edt;
    fctPtrArray[1] = &terminate_edt;
    ocrInit(&argc, argv, 2, fctPtrArray);

    ocrGuid_t outputEventGuid;
    ocrEventCreate(&outputEventGuid, OCR_EVENT_STICKY_T, true);

    // ocrGuid_t terminateEdtGuid;
    // ocrEdtCreate(&terminateEdtGuid, terminate_edt, /*paramc=*/0, /*params=*/ NULL, /*paramv=*/NULL, /*properties=*/0, /*depc=*/1, /*depv=*/NULL, outputEventGuid);
    // ocrAddDependence(outputEventGuid, terminateEdtGuid, 0);
    // ocrEdtSchedule(terminateEdtGuid);

    ocrGuid_t mainEdtGuid;
    ocrEdtCreate(&mainEdtGuid, main_edt, /*paramc=*/0, /*params=*/ NULL,
            /*paramv=*/NULL, /*properties=*/ EDT_PROP_FINISH, /*depc=*/0, NULL, outputEventGuid);
    ocrEdtSchedule(mainEdtGuid);

    ocrCleanup();

    return 0;
}
