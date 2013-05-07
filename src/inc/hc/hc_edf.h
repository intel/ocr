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

#ifndef EDF_H
#define EDF_H

#include "ocr-runtime.h"
#include "ocr-types.h"

/******************************************************/
/* OCR-HC Event Factory                               */
/******************************************************/

typedef struct hc_event_factory {
    ocr_event_factory base_factory;
} hc_event_factory;

struct ocr_event_factory_struct* hc_event_factory_constructor(void);
void hc_event_factory_destructor ( struct ocr_event_factory_struct* base );
ocrGuid_t hcEventFactoryCreate ( struct ocr_event_factory_struct* factory, ocrEventTypes_t eventType, bool takesArg );

typedef struct reg_node_st {
    ocrGuid_t guid;
    int slot;
    struct reg_node_st* next ;
} reg_node_t;

typedef struct hc_event_t {
    ocr_event_t base;
    ocrEventTypes_t kind;
} hc_event_t;

typedef struct hc_event_sticky_t {
    hc_event_t base;
    volatile reg_node_t * waiters;
    volatile reg_node_t * signalers;
    ocrGuid_t data;
} hc_event_sticky_t;

typedef struct hc_event_finishlatch_t {
    hc_event_t base;
    // Dependences to be signaled
    reg_node_t outputEventWaiter;
    reg_node_t parentLatchWaiter; // Parent latch when nesting finish scope
    ocrGuid_t ownerGuid; // finish-edt starting the finish scope
    volatile int counter;
} hc_event_finishlatch_t;

struct ocr_event_struct* hc_event_constructor(ocrEventTypes_t eventType, bool takesArg, ocr_event_fcts_t * event_fct_ptrs_sticky);
void hc_event_destructor ( struct ocr_event_struct* base );

/******************************************************/
/* OCR-HC Task Factory                                */
/******************************************************/

typedef struct hc_task_factory {
    ocr_task_factory base_factory;
} hc_task_factory;

struct ocr_task_factory_struct* hc_task_factory_constructor(void);
void hc_task_factory_destructor ( struct ocr_task_factory_struct* base );

ocrGuid_t hc_task_factory_create ( struct ocr_task_factory_struct* factory, ocrEdt_t fctPtr, u32 paramc, u64 * params, void ** paramv, u16 properties, size_t depc, ocrGuid_t * outputEvent);

/*! \brief Event Driven Task(EDT) implementation for OCR Tasks
*/
typedef struct hc_task_struct_t {
    ocr_task_t base;
    reg_node_t * waiters;
    reg_node_t * signalers; // Does not grow, set once when the task is created
    size_t nbdeps;
    ocrEdt_t p_function;
} hc_task_t;

void hcTaskDestruct ( ocr_task_t* base );
void taskExecute ( ocr_task_t* base );

u64 hc_task_add_acquired(ocr_task_t* base, u64 edtId, ocrGuid_t db);
void hc_task_remove_acquired(ocr_task_t* base, ocrGuid_t db, u64 dbId);

#endif
