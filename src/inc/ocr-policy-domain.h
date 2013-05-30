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

#ifndef OCR_POLICY_DOMAIN_H_
#define OCR_POLICY_DOMAIN_H_

#include "ocr-types.h"
#include "ocr-guid.h"
#include "ocr-scheduler.h"
#include "ocr-comp-target.h"
#include "ocr-worker.h"
#include "ocr-allocator.h"
#include "ocr-mem-platform.h"
#include "ocr-datablock.h"
#include "ocr-sync.h"
#include "ocr-runtime-def.h"
#include "ocr-task-event.h"


/******************************************************/
/* OCR POLICY DOMAIN INTERFACE                        */
/******************************************************/

// Forward declaration
struct ocr_policy_domain_struct;

typedef void (*ocr_policy_create_fct) (struct ocr_policy_domain_struct * policy, void * configuration,
                                       ocrScheduler_t ** schedulers, ocrWorker_t ** workers,
                                       ocr_comp_target_t ** compTargets, ocr_workpile_t ** workpiles,
                                       ocrAllocator_t ** allocators, ocrMemPlatform_t ** memories);
typedef void (*ocr_policy_start_fct) (struct ocr_policy_domain_struct * policy);
typedef void (*ocr_policy_finish_fct) (struct ocr_policy_domain_struct * policy);
typedef void (*ocr_policy_stop_fct) (struct ocr_policy_domain_struct * policy);
typedef void (*ocr_policy_destruct_fct) (struct ocr_policy_domain_struct * policy);
typedef ocrGuid_t (*ocr_policy_getAllocator)(struct ocr_policy_domain_struct *policy, ocrLocation_t* location);

typedef struct ocr_policy_domain_struct {
    ocr_module_t module;
    ocrGuid_t guid;
    int nb_schedulers;
    int nb_workers;
    int nb_comp_targets;
    int nb_workpiles;
    int nb_allocators;
    int nb_memories;

    ocrScheduler_t ** schedulers;
    ocrWorker_t ** workers;
    ocr_comp_target_t ** compTargets;
    ocr_workpile_t ** workpiles;
    ocrAllocator_t ** allocators;
    ocrMemPlatform_t ** memories;

    ocrTaskFactory_t** taskFactories;
    ocrEventFactory_t** eventFactories;

    ocr_policy_create_fct create;
    ocr_policy_start_fct start;
    ocr_policy_finish_fct finish;
    ocr_policy_stop_fct stop;
    ocr_policy_destruct_fct destruct;
    ocr_policy_getAllocator getAllocator;

    void (*receive) (struct ocr_policy_domain_struct * this, ocrGuid_t workerGuid, ocrGuid_t taskGuid);
    void (*handOut) (struct ocr_policy_domain_struct * this, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid);

    ocrGuid_t (*handIn) (struct ocr_policy_domain_struct * this, struct ocr_policy_domain_struct * takingPolicy, ocrGuid_t takingWorkerGuid);
    ocrGuid_t (*extract) (struct ocr_policy_domain_struct * this, struct ocr_policy_domain_struct * takingPolicy, ocrGuid_t takingWorkerGuid);

    ocrTaskFactory_t* (*getTaskFactoryForUserTasks) (struct ocr_policy_domain_struct * policy);
    ocrEventFactory_t* (*getEventFactoryForUserEvents) (struct ocr_policy_domain_struct * policy);

    struct ocr_policy_domain_struct** successors;
    struct ocr_policy_domain_struct** predecessors;
    size_t n_successors;
    size_t n_predecessors;

    //TODO sagnak, HACKY :(
    unsigned char id;
} ocr_policy_domain_t;

/**!
 * Default destructor for ocr policy domain
 */
void ocr_policy_domain_destruct(ocr_policy_domain_t * policy);

/******************************************************/
/* OCR POLICY DOMAIN KINDS AND CONSTRUCTORS           */
/******************************************************/

typedef enum ocr_policy_domain_kind_enum {
    OCR_POLICY_HC = 1,
    OCR_POLICY_XE = 2,
    OCR_POLICY_CE = 3,
    OCR_POLICY_MASTERED_CE = 4,
    OCR_PLACE_POLICY = 5,
    OCR_LEAF_PLACE_POLICY = 6,
    OCR_MASTERED_LEAF_PLACE_POLICY = 7
} ocr_policy_domain_kind;

ocr_policy_domain_t * newPolicyDomain(ocr_policy_domain_kind policyType,
                                size_t nb_workpiles,
                                size_t nb_workers,
                                size_t nb_comp_targets,
                                size_t nb_scheduler);

ocr_policy_domain_t* get_current_policy_domain ();

ocrGuid_t policy_domain_handIn_assert ( ocr_policy_domain_t * this, ocr_policy_domain_t * takingPolicy, ocrGuid_t takingWorkerGuid );
ocrGuid_t policy_domain_extract_assert ( ocr_policy_domain_t * this, ocr_policy_domain_t * takingPolicy, ocrGuid_t takingWorkerGuid );

void policy_domain_handOut_assert ( ocr_policy_domain_t * thisPolicy, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid );
void policy_domain_receive_assert ( ocr_policy_domain_t * thisPolicy, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid );

#endif /* OCR_POLICY_DOMAIN_H_ */
