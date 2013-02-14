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

ocr_policy_domain_t * root_policy;
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

/**!
 * Initialize the OCR runtime.
 * @param argc number of command line options
 * @param argv Pointer to options
 * @param fnc Number of function pointers (UNUSED)
 * @param funcs array of function pointers to be used as edts (UNUSED)
 *
 * Note: removes OCR options from argc / argv
 */
void ocrInit(int * argc, char ** argv, u32 fnc, ocrEdt_t funcs[]) {

    // Intialize the GUID provider
    globalGuidProvider = newGuidProvider(OCR_GUIDPROVIDER_DEFAULT);

    u32 nbHardThreads = ocr_config_default_nb_hardware_threads;
    gHackTotalMemSize = 64*1024*1024; /* 64 MB default */
    char * md_file = parseOcrOptions_MachineDescription(argc, argv);
    if (md_file != NULL) {
        //TODO need a file stat to check
        setMachineDescriptionFromPDL(md_file);
        MachineDescription * md = getMachineDescription();
        if (md == NULL) {
            // Something went wrong when reading the machine description file
            ocr_abort();
        } else {
            nbHardThreads = MachineDescription_getNumHardwareThreads(md);
            gHackTotalMemSize = MachineDescription_getDramSize(md);
        }
    }

    // This is the default policy
    // TODO this should be declared in the default policy model
    size_t nb_workers = nbHardThreads;
    size_t nb_workpiles = nbHardThreads;
    size_t nb_executors = nbHardThreads;
    size_t nb_schedulers = 1;

    ocr_model_policy_t * policy_model = defaultOcrModelPolicy(nb_schedulers, nb_workers,
            nb_executors, nb_workpiles);

    //TODO LIMITATION for now support only one policy
    root_policy = instantiateModel(policy_model);

    root_policy->start(root_policy);
}

void ocrFinish() {
    root_policy->finish(root_policy);
}

void ocrCleanup() {
    // Stop the root policy
    root_policy->stop(root_policy);

    // Now on, there is only thread '0'
    root_policy->destruct(root_policy);
    globalGuidProvider->destruct(globalGuidProvider);
}
