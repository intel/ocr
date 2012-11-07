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

#include "ocr-scheduler.h"
#include "ocr-executor.h"
#include "ocr-low-workers.h"

/******************************************************/
/* OCR POLICY DOMAIN KINDS                            */
/******************************************************/

typedef enum ocr_policy_kind_enum {
    OCR_POLICY_HC = 1
} ocr_policy_kind;

/******************************************************/
/* OCR POLICY DOMAIN INTERFACE                        */
/******************************************************/

// Forward declaration
struct ocr_policy_domain_struct;

typedef void (*ocr_policy_create_fct) (struct ocr_policy_domain_struct * policy, void * configuration,
        ocr_scheduler_t ** schedulers, ocr_worker_t ** workers,
        ocr_executor_t ** executors, ocr_workpile_t ** workpiles);
typedef void (*ocr_policy_start_fct) (struct ocr_policy_domain_struct * policy);
typedef void (*ocr_policy_stop_fct) (struct ocr_policy_domain_struct * policy);
typedef void (*ocr_policy_destroy_fct) (struct ocr_policy_domain_struct * policy);

typedef struct ocr_policy_domain_struct {
    int nb_schedulers;
    int nb_workers;
    int nb_executors;
    int nb_workpiles;

    ocr_scheduler_t ** schedulers;
    ocr_worker_t ** workers;
    ocr_executor_t ** executors;
    ocr_workpile_t ** workpiles;

    ocr_policy_create_fct create;
    ocr_policy_start_fct start;
    ocr_policy_stop_fct stop;
    ocr_policy_destroy_fct destroy;

} ocr_policy_domain_t;

typedef enum ocr_workpile_kind_enum {
    OCR_DEQUE = 1
} ocr_workpile_kind;

/**
 * Default values are set in ocrInit
 */
extern ocr_executor_kind ocr_executor_default_kind;
extern ocr_worker_kind ocr_worker_default_kind;
extern ocr_scheduler_kind ocr_scheduler_default_kind;
extern ocr_policy_kind ocr_policy_default_kind;
extern ocr_workpile_kind ocr_workpile_default_kind;

#endif /* OCR_POLICY_H_ */
