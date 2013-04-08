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

#ifndef __OCR_WORKPILE_H_
#define __OCR_WORKPILE_H_

#include "ocr-guid.h"
#include "ocr-runtime-def.h"


/****************************************************/
/* OCR WORKPILE API                                 */
/****************************************************/

//Forward declaration
struct ocr_workpile_struct;

typedef void (*workpile_create_fct) ( struct ocr_workpile_struct* workpile, void * configuration );
typedef void (*workpile_destruct_fct)(struct ocr_workpile_struct* base);
typedef ocrGuid_t (*workpile_pop_fct) ( struct ocr_workpile_struct* base );
typedef void (*workpile_push_fct) ( struct ocr_workpile_struct* base, ocrGuid_t g );
typedef ocrGuid_t (*workpile_steal_fct) ( struct ocr_workpile_struct* base );

/*! \brief Abstract class to represent OCR task pool data structures.
 *
 *  This class provides the interface for the underlying implementation to conform.
 *  As we want to support work stealing, we current have pop, push and steal interfaces
 */
//TODO We may be influenced by how STL resolves this issue as in push_back, push_front, pop_back, pop_front
typedef struct ocr_workpile_struct {
    ocr_module_t module;

    /*! \brief Creates an concrete implementation of a WorkPool
     *  \return Pointer to the concrete WorkPool that is created by this call
     */
    workpile_create_fct create;
    /*! \brief Virtual destructor for the WorkPool interface
     *  As this class does not have any state, the virtual destructor does not do anything
     */
    workpile_destruct_fct destruct;
    /*! \brief Interface to extract a task from this pool
     *  \return GUID of the task that is extracted from this task pool
     */
    workpile_pop_fct pop;
    /*! \brief Interface to enlist a task
     *  \param[in]  task_guid   GUID of the task that is to be pushed into this task pool.
     */
    workpile_push_fct push;
    /*! \brief Interface to alternative extract a task from this pool
     *  \return GUID of the task that is extracted from this task pool
     */
    workpile_steal_fct steal;
} ocr_workpile_t;


/****************************************************/
/* OCR WORKPILE KINDS AND CONSTRUCTORS              */
/****************************************************/

typedef enum ocr_workpile_kind_enum {
    OCR_DEQUE = 1,
    OCR_MESSAGE_QUEUE = 2
} ocr_workpile_kind;

ocr_workpile_t * newWorkpile(ocr_workpile_kind workpileType);

ocr_workpile_t * hc_workpile_constructor(void);


/****************************************************/
/* OCR WORKPILE ITERATOR API                        */
/****************************************************/

/* Forward declaration */
struct workpile_iterator_struct;

typedef void (*workpile_iterator_reset_fct) (struct workpile_iterator_struct*);
typedef bool (*workpile_iterator_hasNext_fct) (struct workpile_iterator_struct*);
typedef ocr_workpile_t * (*workpile_iterator_next_fct) (struct workpile_iterator_struct*);

typedef struct workpile_iterator_struct {
    workpile_iterator_hasNext_fct hasNext;
    workpile_iterator_next_fct next;
    workpile_iterator_reset_fct reset;
    ocr_workpile_t ** array;
    int id;
    int curr;
    int mod;
} workpile_iterator_t;

void workpile_iterator_reset (workpile_iterator_t * base);
bool workpile_iterator_hasNext (workpile_iterator_t * base);
ocr_workpile_t * workpile_iterator_next (workpile_iterator_t * base);
workpile_iterator_t* workpile_iterator_constructor ( int i, size_t n_pools, ocr_workpile_t ** pools );
void workpile_iterator_destructor (workpile_iterator_t* base);



#endif /* __OCR_WORKPILE_H_ */
