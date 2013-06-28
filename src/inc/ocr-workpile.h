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
#include "ocr-mappable.h"
#include "ocr-utils.h"
#include "ocr-tuning.h"


/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

typedef struct _paramListWorkpileFact_t {
    ocrParamList_t base;
} paramListWorkpileFact_t;

typedef struct _paramListWorkpileInst_t {
    ocrParamList_t base;
} paramListWorkpileInst_t;


/****************************************************/
/* OCR WORKPILE                                     */
/****************************************************/

struct _ocrWorkpile_t;

typedef struct _ocrWorkpileFcts_t {
    void (*destruct)(struct _ocrWorkpile_t *self);
    /*! \brief Interface to extract a task from this pool
     *  \return GUID of the task that is extracted from this task pool
     */
    ocrGuid_t (*pop) (struct _ocrWorkpile_t *self, ocrCost_t *cost);

    /*! \brief Interface to alternative extract a task from this pool
     *  \return GUID of the task that is extracted from this task pool
     */
    ocrGuid_t (*steal)(struct _ocrWorkpile_t *self, ocrCost_t *cost);

    /*! \brief Interface to enlist a task
     *  \param[in]  task_guid   GUID of the task that is to be pushed into this task pool.
     */
    void (*push) (struct _ocrWorkpile_t *self, ocrGuid_t g);
} ocrWorkpileFcts_t;

/*! \brief Abstract class to represent OCR task pool data structures.
 *
 *  This class provides the interface for the underlying implementation to conform.
 *  As we want to support work stealing, we current have pop, push and steal interfaces
 */
//TODO We may be influenced by how STL resolves this issue as in push_back, push_front, pop_back, pop_front
typedef struct _ocrWorkpile_t {
    ocrMappable_t module;
    ocrWorkpileFcts_t *fctPtrs;
} ocrWorkpile_t;


/****************************************************/
/* OCR WORKPILE FACTORY                             */
/****************************************************/

typedef struct _ocrWorkpileFactory_t {
    ocrMappable_t module;
    ocrWorkpile_t * (*instantiate) (struct _ocrWorkpileFactory_t * factory, ocrParamList_t *perInstance);

    void (*destruct)(struct _ocrWorkpileFactory_t * factory);
    ocrWorkpileFcts_t workpileFcts;
} ocrWorkpileFactory_t;


/****************************************************/
/* OCR WORKPILE ITERATOR API                        */
/****************************************************/

typedef struct ocrWorkpileIterator_t {
    bool (*hasNext) (struct ocrWorkpileIterator_t*);
    ocrWorkpile_t * (*next) (struct ocrWorkpileIterator_t*);
    void (*reset) (struct ocrWorkpileIterator_t*);
    ocrWorkpile_t ** array;
    u64 id;
    u64 curr;
    u64 mod;
} ocrWorkpileIterator_t;

void workpileIteratorReset (ocrWorkpileIterator_t * base);
bool workpileIteratorHasNext (ocrWorkpileIterator_t * base);
ocrWorkpile_t * workpileIteratorNext (ocrWorkpileIterator_t * base);
ocrWorkpileIterator_t* workpileIteratorConstructor ( u64 id, u64 n_pools, ocrWorkpile_t ** pools );
void workpile_iterator_destructor (ocrWorkpileIterator_t* base);

#endif /* __OCR_WORKPILE_H_ */
