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

#define EMPTY_DATUM_ERROR_MSG "can not put sentinel value for \"uninitialized\" as a value into EDF"

#define UNINITIALIZED_DATA NULL
#define UNINITIALIZED_EVENT_WAITLIST_PTR ((ocr::Task*) -1)

#include "ocr-runtime.h"

/******************************************************/
/* OCR-HC Event Factory                               */
/******************************************************/

typedef struct hc_event_factory {
    ocr_event_factory base_factory;
} hc_event_factory;

struct ocr_event_factory_struct* hc_event_factory_constructor(void);
void hc_event_factory_destructor ( struct ocr_event_factory_struct* base );
ocrGuid_t hc_event_factory_create ( struct ocr_event_factory_struct* factory, ocrEventTypes_t eventType, bool takesArg );

typedef struct register_list_node_t {
    ocrGuid_t task_guid;
    struct register_list_node_t* next ;
} register_list_node_t;

typedef struct hc_event_t {
    ocr_event_t base;
    ocrGuid_t datum;
    volatile register_list_node_t* register_list;
} hc_event_t;

struct ocr_event_struct* hc_event_constructor(ocrEventTypes_t eventType, bool takesArg);
void hc_event_destructor ( struct ocr_event_struct* base );
ocrGuid_t hc_event_get (struct ocr_event_struct* event);
void hc_event_put (struct ocr_event_struct* event, ocrGuid_t db );
bool hc_event_register_if_not_ready(struct ocr_event_struct* event, ocrGuid_t polling_task_id );

/*! \brief Dependency list data structure for EDTs
*/
typedef struct {
    /*! Public member for array head*/
    ocr_event_t** array;
    /*! Public member for waiting frontier*/
    ocr_event_t** waitingFrontier;
} hc_await_list_t;

/*! \brief Gets the GUID for an OCR entity instance
 *  \param[in] el User provided EventList to build an AwaitList from
 *  \return AwaitList that is a copy of the EventList
 *  Our current implementation copies the linked list into an array
 */
hc_await_list_t* hc_await_list_constructor( size_t al_size );
void hc_await_list_destructor(hc_await_list_t*);

struct hc_task_base_struct_t; 
struct hc_comm_task_struct_t;
typedef struct hc_comm_task_struct_t* (*task_cast_to_comm_fct) ( struct hc_task_base_struct_t* base );

typedef struct hc_task_base_struct_t {
    ocr_task_t base;
    task_cast_to_comm_fct cast_to_comm;
} hc_task_base_t;

typedef struct hc_comm_task_struct_t {
    hc_task_base_t hc_base;
} hc_comm_task_t;

/*! \brief Event Driven Task(EDT) implementation for OCR Tasks
*/
typedef struct hc_task_struct_t {
    hc_task_base_t hc_base;
    hc_await_list_t* awaitList;
    size_t nbdeps;
    ocrEdtDep_t * depv;
    ocrEdt_t p_function;
} hc_task_t;

hc_task_t* hc_task_construct_with_event_list (ocrEdt_t funcPtr, event_list_t* al);
hc_task_t* hc_task_construct (ocrEdt_t funcPtr, size_t l_size);

void hc_task_destruct ( ocr_task_t* base );
bool hc_task_iterate_waiting_frontier ( ocr_task_t* base );
void hc_task_execute ( ocr_task_t* base );
void hc_task_schedule( ocr_task_t* base, ocrGuid_t wid);
void hc_task_add_dependency ( ocr_task_t* base, ocr_event_t* dep, size_t index );

#endif
