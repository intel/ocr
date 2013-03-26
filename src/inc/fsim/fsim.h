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

#ifndef FSIM_H_
#define FSIM_H_

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "ocr-runtime.h"
#include "deque.h"

/******************************************************/
/* OCR-FSIM SCHEDULERS                                */
/******************************************************/

typedef struct {
    ocr_scheduler_t scheduler;
    size_t n_pools;
    ocr_workpile_t ** pools;
/*needed for a naive worker_id to workpile mapping*/
    int n_workers_per_scheduler;
} xe_scheduler_t;

typedef struct {
    ocr_scheduler_t scheduler;
    size_t n_pools;
    ocr_workpile_t ** pools;
/*needed for a naive worker_id to workpile mapping*/
    int n_workers_per_scheduler;
} ce_scheduler_t;

ocr_scheduler_t * xe_scheduler_constructor(void);
ocr_scheduler_t * ce_scheduler_constructor(void);

/**
 * The computation worker routine that asks work to the scheduler
 */
extern void * xe_worker_computation_routine(void * arg);
extern void * ce_worker_computation_routine(void * arg);

/******************************************************/
/* OCR-FSIM Task Factory                              */
/******************************************************/

typedef struct fsim_task_factory {
    ocr_task_factory base_factory;
} fsim_task_factory;

struct ocr_task_factory_struct* fsim_task_factory_constructor(void);
void fsim_task_factory_destructor ( struct ocr_task_factory_struct* base );

/******************************************************/
/* OCR-FSIM Event Factory                             */
/******************************************************/

typedef struct fsim_event_factory {
    ocr_event_factory base_factory;
} fsim_event_factory;

struct ocr_event_factory_struct* fsim_event_factory_constructor(void);
void fsim_event_factory_destructor ( struct ocr_event_factory_struct* base );
ocrGuid_t fsim_event_factory_create ( struct ocr_event_factory_struct* factory, ocrEventTypes_t eventType, bool takesArg );

typedef struct {
    ocr_event_t** array;
    ocr_event_t** waitingFrontier;
} fsim_await_list_t;

fsim_await_list_t* fsim_await_list_constructor( size_t al_size );
void fsim_await_list_destructor(fsim_await_list_t*);

typedef struct fsim_task_struct_t {
    ocr_task_t base;
    fsim_await_list_t* awaitList;
    size_t nbdeps;
    ocrEdtDep_t * depv;
    ocrEdt_t p_function;
} fsim_task_t;

fsim_task_t* fsim_task_construct (ocrEdt_t funcPtr, u32 paramc, u64 * params, void ** paramv, size_t l_size);

typedef struct register_list_node_t {
    ocrGuid_t task_guid;
    struct register_list_node_t* next ;
} register_list_node_t;

typedef struct fsim_event_t {
    ocr_event_t base;
    ocrGuid_t datum;
    volatile register_list_node_t* register_list;
} fsim_event_t;

#endif /* FSIM_H_ */
