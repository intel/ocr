/**
 * @brief OCR interface to computation platforms
 **/

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

/**
 * @brief Parameter list to create a comp-platform factory
 */
typedef struct _paramListCompPlatformFact_t {
    ocrParamList_t base;    /**< Base class */
} paramListCompPlatformFact_t;

/**
 * @brief Parameter list to create a comp-platform instance
 */
typedef struct _paramListCompPlatformInst_t {
    ocrParamList_t base;    /**< Base class */
} paramListCompPlatformInst_t;


/****************************************************/
/* OCR COMPUTE PLATFORM                             */
/****************************************************/

struct _ocrCompPlatform_t;

/**
 * @brief Comp-platform function pointers
 *
 * The function pointers are separate from the comp-platform instance to allow for
 * the sharing of function pointers for comp-platform from the same factory
 */
typedef struct _ocrCompPlatformFcts_t {
    /*! \brief Destroys a comp-platform
     */
    void (*destruct)(struct _ocrCompPlatform_t *self);

    /**
     * @brief Starts a comp-platform (a thread of execution).
     *
     * @param self          Pointer to this comp-platform
     * @param PD            The policy domain bringing up the runtime
     * @param launchArgs    Arguments to be passed down to the comp-platform
     */
    void (*start)(struct _ocrCompPlatform_t *self, struct _ocrPolicyDomain_t * PD, launchArg_t * launchArg);

    /**
     * @brief Stops this comp-platform
     * @param self          Pointer to this comp-platform
     */
    void (*stop)(struct _ocrCompPlatform_t *self);

} ocrCompPlatformFcts_t;

/**
 * @brief Interface to a comp-platform representing a
 * resource able to perform computation.
 */
typedef struct _ocrCompPlatform_t {
    ocrMappable_t module; /**< Base "class" */
    ocrCompPlatformFcts_t *fctPtrs; /**< Function pointers for this instance */
} ocrCompPlatform_t;


/****************************************************/
/* OCR COMPUTE PLATFORM FACTORY                     */
/****************************************************/

/**
 * @brief comp-platform factory
 */
typedef struct _ocrCompPlatformFactory_t {
    /**
     * @brief Instantiate a new comp-platform and returns a pointer to it.
     *
     * @param factory       Pointer to this factory
     * @param instanceArg   Arguments specific for this instance
     */
    ocrCompPlatform_t* (*instantiate)(struct _ocrCompPlatformFactory_t *factory,
                                      ocrParamList_t *instanceArg);

    /**
     * @brief comp-platform factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrCompPlatformFactory_t *factory);

    /**
     * @brief Allows to setup global function pointers
     * @param factory       Pointer to this factory
     */
    void (*setIdentifyingFunctions)(struct _ocrCompPlatformFactory_t *factory);

    ocrCompPlatformFcts_t platformFcts; /**< Function pointers created instances should use */
} ocrCompPlatformFactory_t;

#endif /* __OCR_COMP_PLATFORM_H__ */
