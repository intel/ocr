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

#ifndef OCR_POLICY_H_
#define OCR_POLICY_H_

#include "ocr-types.h"
#include "ocr-guid.h"
#include "ocr-scheduler.h"
#include "ocr-executor.h"
#include "ocr-low-workers.h"
#include "ocr-allocator.h"
#include "ocr-low-memory.h"
#include "ocr-datablock.h"
#include "ocr-sync.h"


/******************************************************/
/* OCR POLICY DOMAIN INTERFACE                        */
/******************************************************/

// Forward declaration
struct ocr_policy_domain_struct;

typedef void (*ocr_policy_create_fct) (struct ocr_policy_domain_struct * policy, void * configuration,
                                       ocr_scheduler_t ** schedulers, ocr_worker_t ** workers,
                                       ocr_executor_t ** executors, ocr_workpile_t ** workpiles,
                                       ocrAllocator_t ** allocators, ocrLowMemory_t ** memories);
typedef void (*ocr_policy_start_fct) (struct ocr_policy_domain_struct * policy);
typedef void (*ocr_policy_finish_fct) (struct ocr_policy_domain_struct * policy);
typedef void (*ocr_policy_stop_fct) (struct ocr_policy_domain_struct * policy);
typedef void (*ocr_policy_destruct_fct) (struct ocr_policy_domain_struct * policy);
typedef ocrGuid_t (*ocr_policy_getAllocator)(struct ocr_policy_domain_struct *policy, ocrLocation_t* location);

typedef struct ocr_policy_domain_struct {
    ocrGuid_t guid;
    int nb_schedulers;
    int nb_workers;
    int nb_executors;
    int nb_workpiles;
    int nb_allocators;
    int nb_memories;

    ocr_scheduler_t ** schedulers;
    ocr_worker_t ** workers;
    ocr_executor_t ** executors;
    ocr_workpile_t ** workpiles;
    ocrAllocator_t ** allocators;
    ocrLowMemory_t ** memories;

    ocr_policy_create_fct create;
    ocr_policy_start_fct start;
    ocr_policy_finish_fct finish;
    ocr_policy_stop_fct stop;
    ocr_policy_destruct_fct destruct;
    ocr_policy_getAllocator getAllocator;

} ocr_policy_domain_t;

/**!
 * Default destructor for ocr policy domain
 */
void ocr_policy_domain_destruct(ocr_policy_domain_t * policy);

/******************************************************/
/* OCR POLICY DOMAIN KINDS AND CONSTRUCTORS           */
/******************************************************/

typedef enum ocr_policy_kind_enum {
    OCR_POLICY_HC = 1
/* sagnak begin */
	,
    OCR_POLICY_FSIM_XE,
    OCR_POLICY_FSIM_CE
/* sagnak end*/
} ocr_policy_kind;

ocr_policy_domain_t * newPolicy(ocr_policy_kind policyType,
        size_t nb_workpiles,
        size_t nb_workers,
        size_t nb_executors,
        size_t nb_scheduler);

 ocr_policy_domain_t * hc_policy_domain_constructor();
 ocr_policy_domain_t * fsim_xe_policy_domain_constructor();
 ocr_policy_domain_t * fsim_ce_policy_domain_constructor();

#endif /* OCR_POLICY_H_ */
