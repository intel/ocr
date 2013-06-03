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
#include <stdio.h>
#include <string.h>

#include "ocr-runtime.h"
#include "ocr-config.h"
#include "ocr-guid.h"

#include "sync/x86/x86.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-stat-user.h"
#endif


size_t n_root_policy_nodes;
ocrPolicyDomain_t ** root_policies;

// the runtime fork/join-er
ocrWorker_t* master_worker;
u64 gHackTotalMemSize;

//TODO we should have an argument option parsing library
/**!
 * Utility function to remove ocr arguments before handing argv
 * over to the user program
 */
static void shift_arguments(int * argc, char ** argv, int start_offset, int shift_offset) {
    int i = start_offset;
    int j = shift_offset;
    while ( j < *argc) {
        argv[i++] = argv[j++];
    }

    *argc = (*argc - (shift_offset-start_offset));
}

/**!
 * Check if we have a machine description passed as argument
 */
static char * parseOcrOptions_MachineDescription(int * argc, char ** argv) {
    int i = 0;
    char * md_file = NULL;
    while(i < *argc) {
        if (strcmp(argv[i], "-md") == 0) {
            md_file = argv[i+1];
            shift_arguments(argc, argv, i, i+2);
        }
        i++;
    }
    return md_file;
}

void ocrInit(int * argc, char ** argv, u32 fnc, ocrEdt_t funcs[]) {
    // REC: Hack, for now we just initialize the factories here.
    // See if we need to move some into policy domains and how to
    // initialize them
    // Do this first to make sure that stuff that depends on it works :)
    GocrLockFactory = newLockFactoryX86(NULL);
    GocrAtomic64Factory = newAtomic64FactoryX86(NULL);
    GocrQueueFactory = newQueueFactoryX86(NULL);

#ifdef OCR_ENABLE_STATISTICS
    GocrFilterAggregator = NEW_FILTER(filedump);
    GocrFilterAggregator->create(GocrFilterAggregator, NULL, NULL);

    // HUGE HUGE Hack
    ocrStatsProcessCreate(&GfakeProcess, 0);
#endif

    // Intialize the GUID provider
    globalGuidProvider = newGuidProvider(OCR_GUIDPROVIDER_DEFAULT);
    gHackTotalMemSize = 512*1024*1024; /* 64 MB default */
    char * md_file = parseOcrOptions_MachineDescription(argc, argv);
    if ( md_file != NULL && !strncmp(md_file,"fsim",5) ) {
        ocrInitPolicyModel(OCR_POLICY_MODEL_FSIM, md_file);
    } else if ( md_file != NULL && !strncmp(md_file,"thor",4) ) {
        ocrInitPolicyModel(OCR_POLICY_MODEL_THOR, md_file);
    } else {
        ocrInitPolicyModel(OCR_POLICY_MODEL_FLAT, md_file);
    }
}

void ocrFinish() {
    // This is called by the sink EDT and allow the master_worker
    // to fall-through its worker routine code (see ocrCleanup)
    master_worker->stop(master_worker);
}

static void recursive_policy_finish_helper ( ocrPolicyDomain_t* curr ) {
    if ( curr ) {
        int index = 0; // successor index
        for ( ; index < curr->n_successors; ++index ) {
            recursive_policy_finish_helper(curr->successors[index]);
        }
        curr->finish(curr);
    }
}

static void recursive_policy_stop_helper ( ocrPolicyDomain_t* curr ) {
    if ( curr ) {
        int index = 0; // successor index
        for ( ; index < curr->n_successors; ++index ) {
            recursive_policy_stop_helper(curr->successors[index]);
        }
        curr->stop(curr);
    }
}

static void recursive_policy_destruct_helper ( ocrPolicyDomain_t* curr ) {
    if ( curr ) {
        int index = 0; // successor index
        for ( ; index < curr->n_successors; ++index ) {
            recursive_policy_destruct_helper(curr->successors[index]);
        }
        curr->destruct(curr);
    }
}

static inline void unravel () {
    // current root policy index
    int index = 0;

    for ( index = 0; index < n_root_policy_nodes; ++index ) {
        recursive_policy_finish_helper(root_policies[index]);
    }

    for ( index = 0; index < n_root_policy_nodes; ++index ) {
        recursive_policy_stop_helper(root_policies[index]);
    }

    for ( index = 0; index < n_root_policy_nodes; ++index ) {
        recursive_policy_destruct_helper(root_policies[index]);
    }

    globalGuidProvider->destruct(globalGuidProvider);
    free(root_policies);
}

void ocrCleanup() {
    master_worker->routine(master_worker);
    // master_worker is done executing. 
    // Proceed and stop other workers and the runtime.
    unravel();
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcessDestruct(&GfakeProcess);
    GocrFilterAggregator->destruct(GocrFilterAggregator);
#endif
}
