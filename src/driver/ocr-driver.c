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

#include <stdlib.h>
#include <assert.h>

#include "ocr-runtime.h"

ocr_policy_domain_t * root_policy;

//TODO add argc, argv
void ocrInit() {
    //TODO this is to be obtained from some configuration file or command line
    ocr_worker_default_kind = OCR_WORKER_HC;
    ocr_executor_default_kind = OCR_EXECUTOR_HC;
    ocr_workpile_default_kind = OCR_DEQUE;
    ocr_scheduler_default_kind = OCR_SCHEDULER_WST;
    ocr_policy_default_kind = OCR_POLICY_HC;
    ocrAllocatorDefaultKind = OCR_ALLOCATOR_TLSF;
    ocrLowMemoryDefaultKind = OCR_LOWMEMORY_MALLOC;
    ocrDataBlockDefaultKind = OCR_DATABLOCK_REGULAR;
    ocrLockDefaultKind = OCR_LOCK_X86;

    //TODO this is to be obtained from some configuration file or command line
    size_t nb_workers = 8;
    size_t nb_workpiles = 8;
    size_t nb_executors = 8;
    size_t nb_schedulers = 1;

    ocr_model_policy_t * policy_model = defaultOcrModelPolicy(nb_schedulers, nb_workers,
            nb_executors, nb_workpiles);

    //TODO LIMITATION for now support only one policy
    root_policy = instantiateModel(policy_model);

    root_policy->start(root_policy);
}

void ocrFinish() {
    //TODO this is specific to how policies are stopped so it should
    //go in ocr-policy.c, need to think about naming here

    // Note: As soon as worker '0' is stopped its thread is
    // free to fall-through in ocr_finalize() (see warning there)
    size_t i;
    for ( i = 0; i < root_policy->nb_workers; ++i ) {
        root_policy->workers[i]->stop(root_policy->workers[i]);
    }
}

void ocrCleanup() {
    // Stop the root policy
    root_policy->stop(root_policy);

    // Now on, there is only thread '0'
    root_policy->destruct(root_policy);
}
