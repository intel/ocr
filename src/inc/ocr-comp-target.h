/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_COMP_TARGET_H__
#define __OCR_COMP_TARGET_H__

#include "ocr-mappable.h"
#include "ocr-types.h"
#include "ocr-utils.h"

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

typedef struct _paramListCompTargetFact_t {
    ocrParamList_t base;
} paramListCompTargetFact_t;

typedef struct _paramListCompTargetInst_t {
    ocrParamList_t base;
} paramListCompTargetInst_t;

/****************************************************/
/* OCR COMPUTE TARGET                               */
/****************************************************/

typedef struct _launchArg_t {
  void *(*routine)(void*);
  void * arg;
  struct _ocrPolicyDomain_t * PD;
} launchArg_t;

struct _ocrCompTarget_t;

typedef struct _ocrCompTargetFcts_t {
    void (*destruct) (struct _ocrCompTarget_t * self);

    /*! \brief Starts a compute target
     *
     */
    void (*start) (struct _ocrCompTarget_t * self, struct _ocrPolicyDomain_t * PD, launchArg_t * launchArg);

    /*! \brief Stops this comp-target
     */
    void (*stop) (struct _ocrCompTarget_t * self);
} ocrCompTargetFcts_t;

struct _ocrCompPlatform_t;

/** @brief Abstract class to represent OCR compute-target
 *
 * A compute target will run on a compute-platform and emulates a computing
 * resource at the target level.
 *
 */
typedef struct _ocrCompTarget_t {
    ocrMappable_t module;
    ocrGuid_t guid;

    struct _ocrCompPlatform_t ** platforms; /**< Computing platform this compute target
                                    * is executing on */
    u64 platformCount;

    ocrCompTargetFcts_t *fctPtrs;
} ocrCompTarget_t;

/****************************************************/
/* OCR COMPUTE TARGET FACTORY                       */
/****************************************************/
typedef struct _ocrCompTargetFactory_t {
    ocrCompTarget_t * (*instantiate) ( struct _ocrCompTargetFactory_t * factory,
                                       ocrParamList_t *perInstance);
    void (*destruct)(struct _ocrCompTargetFactory_t * factory);

    ocrCompTargetFcts_t targetFcts;
} ocrCompTargetFactory_t;

#endif /* __OCR_COMP_TARGET_H__ */
