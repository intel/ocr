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

#include "hc.h"

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "ocr-runtime.h"
#include "deque.h"


typedef struct {
    hc_worker_t hcBase;
    pthread_cond_t isRunningCond;
    pthread_mutex_t isRunningMutex;
} xe_worker_t;

typedef struct ce_message_workpile {
    ocr_workpile_t base;
    mpsc_deque_t * deque;
} ce_message_workpile;

/******************************************************/
/* OCR-FSIM SCHEDULERS                                */
/******************************************************/

typedef struct {
    hc_scheduler_t scheduler;
/*needed for a naive worker_id to workpile mapping*/
    int n_workers_per_scheduler;
    size_t worker_id_begin;
    size_t worker_id_end;
} xe_scheduler_t;

typedef struct {
    hc_scheduler_t scheduler;
/*needed for a naive worker_id to workpile mapping*/
    int n_workers_per_scheduler;
    size_t worker_id_begin;
    size_t worker_id_end;

    int in_message_popping_mode;
} ce_scheduler_t;

ocr_scheduler_t * xe_scheduler_constructor(void);
ocr_scheduler_t * ce_scheduler_constructor(void);

/******************************************************/
/* OCR-FSIM Task Factory                              */
/******************************************************/

typedef struct fsim_task_factory {
    ocr_task_factory base_factory;
} fsim_task_factory;

struct ocr_task_factory_struct* fsim_task_factory_constructor(void);
void fsim_task_factory_destructor ( struct ocr_task_factory_struct* base );

typedef struct fsim_message_task_factory {
    ocr_task_factory base_factory;
} fsim_message_task_factory;

struct ocr_task_factory_struct* fsim_message_task_factory_constructor(void);
void fsim_message_task_factory_destructor ( struct ocr_task_factory_struct* base );

typedef struct fsim_message_interface_struct {
    int (*is_message) ( struct fsim_message_interface_struct *);
} fsim_message_interface_t;

typedef struct fsim_task_base_struct_t {
    hc_task_t base;
    fsim_message_interface_t message_interface;
} fsim_task_base_t;

typedef struct fsim_task_struct_t {
    fsim_task_base_t fsimBase;
} fsim_task_t;

fsim_task_t* fsim_task_construct (ocrEdt_t funcPtr, u32 paramc, u64 * params, void ** paramv, size_t l_size);

typedef enum message_type_enum { PICK_MY_WORK_UP, GIVE_ME_WORK } message_type;

typedef struct fsim_message_task_struct_t {
    fsim_task_base_t fsimBase;
    message_type type;
    ocrGuid_t from_worker_guid;
} fsim_message_task_t;

fsim_message_task_t* fsim_message_task_construct (ocrEdt_t funcPtr);

typedef hc_event_t fsim_event_t;

#endif /* FSIM_H_ */
