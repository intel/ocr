/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_WORKPILE_H_
#define __OCR_WORKPILE_H_

#include "ocr-mappable.h"
#include "ocr-tuning.h"
#include "ocr-types.h"
#include "ocr-utils.h"


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
struct _ocrPolicyDomain_t;

typedef struct _ocrWorkpileFcts_t {
    void (*destruct)(struct _ocrWorkpile_t *self);

    void (*start)(struct _ocrWorkpile_t *self, struct _ocrPolicyDomain_t *PD);

    void (*stop)(struct _ocrWorkpile_t *self);

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
ocrWorkpileIterator_t* newWorkpileIterator( u64 id, u64 workpileCount, ocrWorkpile_t ** workpiles );
void workpileIteratorDestruct(ocrWorkpileIterator_t* base);

#endif /* __OCR_WORKPILE_H_ */
