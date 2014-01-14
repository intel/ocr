/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_WORKPILE_H_
#define __OCR_WORKPILE_H_

#include "ocr-runtime-types.h"
#include "ocr-tuning.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

struct _ocrPolicyDomain_t;


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

    void (*finish)(struct _ocrWorkpile_t *self);

    /** @brief Interface to extract a task from this pool
     *
     *  This will get a task from the workpile.
     *  @param[in] self         Pointer to this workpile
     *  @param[in] type         Type of pop (regular or steal for eg)
     *  @param[in] cost         Cost function to use to perform the pop
     *
     *  @return GUID of the task extracted from the task pool. After the
     *  call, that task will no longer exist in the pool
     *  @todo cost is not used as of now
     */
    ocrFatGuid_t (*pop)(struct _ocrWorkpile_t *self, ocrWorkPopType_t type,
                        ocrCost_t *cost);

    /** @brief Interface to push a task into the pool
     *  @param[in] self         Pointer to this workpile
     *  @param[in] type         Type of push
     *  @param[in] g            GUID of task to push
     */
    void (*push)(struct _ocrWorkpile_t *self, ocrWorkPushType_t type,
                 ocrFatGuid_t g);
} ocrWorkpileFcts_t;

/*! \brief Abstract class to represent OCR task pool data structures.
 *
 *  This class provides the interface for the underlying implementation to conform.
 *  As we want to support work stealing, we current have pop, push and steal interfaces
 */
//TODO We may be influenced by how STL resolves this issue as in push_back, push_front, pop_back, pop_front
typedef struct _ocrWorkpile_t {
    ocrFatGuid_t fguid;
    struct _ocrPolicyDomain_t *pd;
    ocrWorkpileFcts_t fcts;
} ocrWorkpile_t;


/****************************************************/
/* OCR WORKPILE FACTORY                             */
/****************************************************/

typedef struct _ocrWorkpileFactory_t {
    ocrWorkpile_t * (*instantiate)(struct _ocrWorkpileFactory_t * factory,
                                   ocrParamList_t *perInstance);

    void (*destruct)(struct _ocrWorkpileFactory_t * factory);
    ocrWorkpileFcts_t workpileFcts;
} ocrWorkpileFactory_t;

#endif /* __OCR_WORKPILE_H_ */
