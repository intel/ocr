/**
 * @brief OCR interface to computation platforms
 **/

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

/**
 * @brief Parameter list to create a comp-target factory
 */
typedef struct _paramListCompTargetFact_t {
    ocrParamList_t base;  /**< Base class */
} paramListCompTargetFact_t;

/**
 * @brief Parameter list to create a comp-target instance
 */
typedef struct _paramListCompTargetInst_t {
    ocrParamList_t base;  /**< Base class */
} paramListCompTargetInst_t;


/****************************************************/
/* OCR COMPUTE TARGET                               */
/****************************************************/

/**
 * @brief Structure to store arguments when starting underlying
 * comp-platforms.
 */
typedef struct _launchArg_t {
  void *(*routine)(void*); /**< function pointer to execute */
  void * arg; /**< argument for the function pointer */
  struct _ocrPolicyDomain_t * PD; /**< The policy domain the comp-target belongs to */
} launchArg_t;

struct _ocrCompTarget_t;

/**
 * @brief comp-target function pointers
 *
 * The function pointers are separate from the comp-target instance to allow for
 * the sharing of function pointers for comp-target from the same factory
 */
typedef struct _ocrCompTargetFcts_t {
    /*! \brief Destroys a comp-target
     */
    void (*destruct) (struct _ocrCompTarget_t * self);

    /**
     * @brief Starts a comp-target
     *
     * @param self          Pointer to this comp-target
     * @param PD            The policy domain the comp-target belongs to
     * @param launchArgs    Arguments to be passed to the underlying comp-platform
     */
    void (*start) (struct _ocrCompTarget_t * self, struct _ocrPolicyDomain_t * PD, launchArg_t * launchArg);

    /*! \brief Stops this comp-target
     */
    void (*stop) (struct _ocrCompTarget_t * self);
} ocrCompTargetFcts_t;

struct _ocrCompPlatform_t;

/** @brief Abstract class to represent OCR compute-target
 *
 * A compute target runs on some compute-platforms and 
 * emulates a computing resource at the target level.
 * This is typically a one-one mapping but it's not mandatory.
 */
typedef struct _ocrCompTarget_t {
    ocrMappable_t module; /**< Base class */
    ocrGuid_t guid; /**< Guid of the comp-target */
    struct _ocrCompPlatform_t ** platforms; /**< Computing platform the compute target
                                             * is executing on */
    u64 platformCount; /**< Number of comp-platforms */
    ocrCompTargetFcts_t *fctPtrs; /**< Function pointers for this instance */
} ocrCompTarget_t;


/****************************************************/
/* OCR COMPUTE TARGET FACTORY                       */
/****************************************************/

/**
 * @brief comp-target factory
 */
typedef struct _ocrCompTargetFactory_t {
   /**
     * @brief comp-target factory
     *
     * Initiates a new comp-target and returns a pointer to it.
     *
     * @param factory       Pointer to this factory
     * @param instanceArg   Arguments specific for this instance
     */
    ocrCompTarget_t * (*instantiate) ( struct _ocrCompTargetFactory_t * factory,
                                       ocrParamList_t *instanceArg);
    /**
     * @brief comp-target factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrCompTargetFactory_t * factory);

    ocrCompTargetFcts_t targetFcts;  /**< Function pointers created instances should use */
} ocrCompTargetFactory_t;

#endif /* __OCR_COMP_TARGET_H__ */
