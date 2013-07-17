/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_COMP_PLATFORM_H__
#define __OCR_COMP_PLATFORM_H__

#include "ocr-comp-target.h"
#include "ocr-mappable.h"
#include "ocr-types.h"
#include "ocr-utils.h"


/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

typedef struct _paramListCompPlatformFact_t {
    ocrParamList_t base;
} paramListCompPlatformFact_t;

typedef struct _paramListCompPlatformInst_t {
    ocrParamList_t base;
} paramListCompPlatformInst_t;


/****************************************************/
/* OCR COMPUTE PLATFORM                             */
/****************************************************/

struct _ocrCompPlatform_t;

typedef struct _ocrCompPlatformFcts_t {
    void (*destruct)(struct _ocrCompPlatform_t *self);
    /**
     * @brief Starts a thread of execution.
     *
     * The function started will be 'routine' and it will be passed 'routineArg'
     * @todo There was something about a stack size...
     */
    void (*start)(struct _ocrCompPlatform_t *self, struct _ocrPolicyDomain_t * PD, launchArg_t * launchArg);

    /**
     * @brief Stops this tread of execution
     */
    void (*stop)(struct _ocrCompPlatform_t *self);

} ocrCompPlatformFcts_t;

typedef struct _ocrCompPlatform_t {
    ocrMappable_t module;
    ocrCompPlatformFcts_t *fctPtrs;
} ocrCompPlatform_t;


/****************************************************/
/* OCR COMPUTE PLATFORM FACTORY                     */
/****************************************************/

typedef struct _ocrCompPlatformFactory_t {
    ocrCompPlatform_t* (*instantiate)(struct _ocrCompPlatformFactory_t *factory,
                                      ocrParamList_t *perInstance);

    void (*destruct)(struct _ocrCompPlatformFactory_t *factory);

    void (*setIdentifyingFunctions)(struct _ocrCompPlatformFactory_t *factory);

    ocrCompPlatformFcts_t platformFcts;
} ocrCompPlatformFactory_t;

#endif /* __OCR_COMP_PLATFORM_H__ */
