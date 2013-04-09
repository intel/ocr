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

size_t n_root_policy_nodes;
ocr_policy_domain_t ** root_policies;

// the runtime fork/join-er
ocr_worker_t* master_worker;
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

    /* sagnak begin */
    if ( md_file != NULL && !strncmp(md_file,"fsim",5) ) {
        // sagnak TODO handle nb_CEs <= 1 case
        size_t nb_CEs = 2;
        size_t nb_XE_per_CEs = 3;
        size_t nb_XEs = nb_XE_per_CEs * nb_CEs;

	ocr_model_policy_t * xe_policy_models = createXeModelPolicies ( nb_CEs, nb_XE_per_CEs );
	ocr_model_policy_t * ce_policy_models = NULL;
        if ( nb_CEs > 1 )
            ce_policy_models = createCeModelPolicies ( nb_CEs-1, nb_XE_per_CEs );
	ocr_model_policy_t * ce_mastered_policy_model = createCeMasteredModelPolicy( nb_XE_per_CEs );

	ocr_policy_domain_t ** xe_policy_domains = instantiateModel(xe_policy_models);
	ocr_policy_domain_t ** ce_policy_domains = NULL;
        if ( nb_CEs > 1 )
            ce_policy_domains = instantiateModel(ce_policy_models);
	ocr_policy_domain_t ** ce_mastered_policy_domain = instantiateModel(ce_mastered_policy_model);

        n_root_policy_nodes = nb_CEs;
        root_policies = (ocr_policy_domain_t**) malloc(sizeof(ocr_policy_domain_t*)*nb_CEs);

        root_policies[0] = ce_mastered_policy_domain[0];

        size_t idx = 0;
        for ( idx = 0; idx < nb_CEs-1; ++idx ) {
            root_policies[idx+1] = ce_policy_domains[idx];
        }

	idx = 0;
	ce_mastered_policy_domain[0]->n_successors = nb_XE_per_CEs;
	ce_mastered_policy_domain[0]->successors = &(xe_policy_domains[idx*nb_XE_per_CEs]);
	ce_mastered_policy_domain[0]->n_predecessors = 0;
	ce_mastered_policy_domain[0]->predecessors = NULL;

	for ( idx = 1; idx < nb_CEs; ++idx ) {
	    ocr_policy_domain_t *curr = ce_policy_domains[idx-1];

	    curr->n_successors = nb_XE_per_CEs;
	    curr->successors = &(xe_policy_domains[idx*nb_XE_per_CEs]);
	    curr->n_predecessors = 0;
	    curr->predecessors = NULL;
	}

	// sagnak: should this instead be recursive?
	for ( idx = 0; idx < nb_XE_per_CEs; ++idx ) {
	    ocr_policy_domain_t *curr = xe_policy_domains[idx];

	    curr->n_successors = 0;
	    curr->successors = NULL;
	    curr->n_predecessors = 1;
	    curr->predecessors = ce_mastered_policy_domain;
	}

	for ( idx = nb_XE_per_CEs; idx < nb_XEs; ++idx ) {
	    ocr_policy_domain_t *curr = xe_policy_domains[idx];

	    curr->n_successors = 0;
	    curr->successors = NULL;
	    curr->n_predecessors = 1;
	    curr->predecessors = &(ce_policy_domains[idx/nb_XE_per_CEs-1]);
	}

	ce_mastered_policy_domain[0]->start(ce_mastered_policy_domain[0]);

	for ( idx = 1; idx < nb_CEs; ++idx ) {
	    ocr_policy_domain_t *curr = ce_policy_domains[idx-1];
	    curr->start(curr);
    }
	for ( idx = 0; idx < nb_XEs; ++idx ) {
	    ocr_policy_domain_t *curr = xe_policy_domains[idx];
	    curr->start(curr);
    }

	master_worker = ce_mastered_policy_domain[0]->workers[0];

    } else {
	if (md_file != NULL) {
	    //TODO need a file stat to check
	    setMachineDescriptionFromPDL(md_file);
	    MachineDescription * md = getMachineDescription();
	    if (md == NULL) {
		// Something went wrong when reading the machine description file
		ocr_abort();
	    } else {
		nbHardThreads = MachineDescription_getNumHardwareThreads(md);
                //	gHackTotalMemSize = MachineDescription_getDramSize(md);
	    }
	}

	// This is the default policy
	// TODO this should be declared in the default policy model
	size_t nb_policy_domain = 1;
	size_t nb_workers_per_policy_domain = nbHardThreads;
	size_t nb_workpiles_per_policy_domain = nbHardThreads;
	size_t nb_executors_per_policy_domain = nbHardThreads;
	size_t nb_schedulers_per_policy_domain = 1;

	ocr_model_policy_t * policy_model = defaultOcrModelPolicy(nb_policy_domain,
		nb_schedulers_per_policy_domain, nb_workers_per_policy_domain,
		nb_executors_per_policy_domain, nb_workpiles_per_policy_domain);

	//TODO LIMITATION for now support only one policy
	n_root_policy_nodes = nb_policy_domain;
	root_policies = instantiateModel(policy_model);

	master_worker = root_policies[0]->workers[0];
	root_policies[0]->start(root_policies[0]);
    }
}

static void recursive_policy_finish_helper ( ocr_policy_domain_t* curr ) {
    if ( curr ) {
	int index = 0; // successor index
	for ( ; index < curr->n_successors; ++index ) {
	    recursive_policy_finish_helper(curr->successors[index]);
	}
	curr->finish(curr);
    }
}

static void recursive_policy_stop_helper ( ocr_policy_domain_t* curr ) {
    if ( curr ) {
	int index = 0; // successor index
	for ( ; index < curr->n_successors; ++index ) {
	    recursive_policy_stop_helper(curr->successors[index]);
	}
	curr->stop(curr);
    }
}

void ocrFinish() {
    master_worker->stop(master_worker);
}

static void recursive_policy_destruct_helper ( ocr_policy_domain_t* curr ) {
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
}

void ocrCleanup() {
    master_worker->routine(master_worker);

    unravel();
}
