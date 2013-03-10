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

#ifndef __OCR_SCHEDULER_H__
#define __OCR_SCHEDULER_H__

#include "ocr-guid.h"
#include "ocr-types.h"
#include "ocr-utils.h"
#include "ocr-workpile.h"
#include "ocr-runtime-def.h"


/****************************************************/
/* OCR SCHEDULER API                                */
/****************************************************/

// forward declarations
struct ocr_worker_struct;
struct ocr_scheduler_struct;

typedef void (*scheduler_create_fct) (struct ocr_scheduler_struct*, void * configuration);
typedef void (*scheduler_destruct_fct) (struct ocr_scheduler_struct*);

typedef ocr_workpile_t * (*scheduler_pop_mapping_fct) (struct ocr_scheduler_struct*, struct ocr_worker_struct*);
typedef ocr_workpile_t * (*scheduler_push_mapping_fct) (struct ocr_scheduler_struct*, struct ocr_worker_struct*);
typedef workpile_iterator_t* (*scheduler_steal_mapping_fct) (struct ocr_scheduler_struct*, struct ocr_worker_struct*);

/*! \brief Interface to extract a task from this scheduler
 *  \param[in]  worker_guid GUID of the worker trying to extract a task from this scheduler
 *  \return GUID of the task that is extracted from this task pool
 */
typedef ocrGuid_t (*scheduler_take_fct) (struct ocr_scheduler_struct * , ocrGuid_t wid );

/*! \brief Interface to provide a task to this scheduler
 *  \param[in]  worker_guid GUID of the worker providing a task to this scheduler
 *  \param[in]  task_guid   GUID of the task being given to this scheduler
 */
typedef void (*scheduler_give_fct) (struct ocr_scheduler_struct * , ocrGuid_t wid, ocrGuid_t tid );

// forward declaration
struct ocr_policy_domain_struct;

/*! \brief Represents OCR schedulers.
 *
 *  Currently, we allow scheduler interface to have work taken from them or given to them
 */
typedef struct ocr_scheduler_struct {
    ocr_module_t module;
    scheduler_create_fct create;
    scheduler_destruct_fct destruct;
    scheduler_pop_mapping_fct pop_mapping;
    scheduler_push_mapping_fct push_mapping;
    scheduler_steal_mapping_fct steal_mapping;
    scheduler_take_fct take;
    scheduler_give_fct give;
    ocr_module_map_fct map;
    struct ocr_policy_domain_struct* domain;
} ocr_scheduler_t;


/****************************************************/
/* OCR SCHEDULER KINDS AND CONSTRUCTORS             */
/****************************************************/

typedef enum ocr_scheduler_kind_enum {
    OCR_SCHEDULER_WST = 1
	, OCR_SCHEDULER_XE = 2
	, OCR_SCHEDULER_CE = 3
} ocr_scheduler_kind;

ocr_scheduler_t * newScheduler(ocr_scheduler_kind schedulerType);

ocr_scheduler_t * hc_scheduler_constructor(void);

ocr_scheduler_t * xe_scheduler_constructor(void);

ocr_scheduler_t * ce_scheduler_constructor(void);

#endif /* __OCR_SCHEDULER_H__ */
