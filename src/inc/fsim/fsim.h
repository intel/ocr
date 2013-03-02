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
} xe_scheduler_t;

typedef struct {
    ocr_scheduler_t scheduler;
    size_t n_pools;
    ocr_workpile_t ** pools;
} ce_scheduler_t;

ocr_scheduler_t * xe_scheduler_constructor(void);
ocr_scheduler_t * ce_scheduler_constructor(void);

/******************************************************/
/* OCR-FSIM Task Factory                              */
/******************************************************/

typedef struct xe_task_factory {
    ocr_task_factory base_factory;
} xe_task_factory;

struct ocr_task_factory_struct* xe_task_factory_constructor(void);
void xe_task_factory_destructor ( struct ocr_task_factory_struct* base );

typedef struct ce_task_factory {
    ocr_task_factory base_factory;
} ce_task_factory;

struct ocr_task_factory_struct* ce_task_factory_constructor(void);
void ce_task_factory_destructor ( struct ocr_task_factory_struct* base );

/**
 * The computation worker routine that asks work to the scheduler
 */
extern void * xe_worker_computation_routine(void * arg);
extern void * ce_worker_computation_routine(void * arg);

/******************************************************/
/* OCR-FSIM Event Factory                             */
/******************************************************/

typedef struct xe_event_factory {
    ocr_event_factory base_factory;
} xe_event_factory;

struct ocr_event_factory_struct* xe_event_factory_constructor(void);
void xe_event_factory_destructor ( struct ocr_event_factory_struct* base );
ocrGuid_t xe_event_factory_create ( struct ocr_event_factory_struct* factory, ocrEventTypes_t eventType, bool takesArg );

typedef struct ce_event_factory {
    ocr_event_factory base_factory;
} ce_event_factory;

struct ocr_event_factory_struct* ce_event_factory_constructor(void);
void ce_event_factory_destructor ( struct ocr_event_factory_struct* base );
ocrGuid_t ce_event_factory_create ( struct ocr_event_factory_struct* factory, ocrEventTypes_t eventType, bool takesArg );

typedef struct {
    ocr_event_t** array;
    ocr_event_t** waitingFrontier;
} xe_await_list_t;

xe_await_list_t* xe_await_list_constructor( size_t al_size );
void xe_await_list_destructor(xe_await_list_t*);

typedef struct xe_task_struct_t {
    ocr_task_t base;
    xe_await_list_t* awaitList;
    size_t nbdeps;
    ocrEdtDep_t * depv;
    ocrEdt_t p_function;
} xe_task_t;

xe_task_t* xe_task_construct (ocrEdt_t funcPtr, u32 paramc, u64 * params, void ** paramv, size_t l_size);

typedef struct {
    ocr_event_t** array;
    ocr_event_t** waitingFrontier;
} ce_await_list_t;

ce_await_list_t* ce_await_list_constructor( size_t al_size );
void ce_await_list_destructor(ce_await_list_t*);

typedef struct ce_task_struct_t {
    ocr_task_t base;
    ce_await_list_t* awaitList;
    size_t nbdeps;
    ocrEdtDep_t * depv;
    ocrEdt_t p_function;
} ce_task_t;

ce_task_t* ce_task_construct (ocrEdt_t funcPtr, u32 paramc, u64 * params, void ** paramv, size_t l_size);

typedef struct register_list_node_t {
    ocrGuid_t task_guid;
    struct register_list_node_t* next ;
} register_list_node_t;

typedef struct xe_event_t {
    ocr_event_t base;
    ocrGuid_t datum;
    volatile register_list_node_t* register_list;
} xe_event_t;

typedef struct ce_event_t {
    ocr_event_t base;
    ocrGuid_t datum;
    volatile register_list_node_t* register_list;
} ce_event_t;


#endif /* FSIM_H_ */
