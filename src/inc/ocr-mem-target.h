/**
 * @brief OCR interface to the target level memory interface
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_MEM_TARGET_H__
#define __OCR_MEM_TARGET_H__

#include "ocr-mappable.h"
#include "ocr-types.h"
#include "ocr-utils.h"


/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

/**
 * @brief Parameter list to create a mem-target factory
 */
typedef struct _paramListMemTargetFact_t {
    ocrParamList_t base;
} paramListMemTargetFact_t;

/**
 * @brief Parameter list to create a mem-target instance
 */
typedef struct _paramListMemTargetInst_t {
    ocrParamList_t base;
} paramListMemTargetInst_t;


/****************************************************/
/* OCR MEMORY TARGETS                               */
/****************************************************/

struct _ocrMemTarget_t;
struct _ocrPolicyDomain_t;

typedef struct _ocrMemTargetFcts_t {
    /*! \brief Destroys a mem-target
     *  \param[in] self          Pointer to this mem-target
     */
    void (*destruct)(struct _ocrMemTarget_t* self);

    /*! \brief Starts the mem-target
     *  \param[in] self          Pointer to this mem-target
     *  \param[in] PD            Current policy domain
     */
    void (*start)(struct _ocrMemTarget_t* self, struct _ocrPolicyDomain_t * PD);

    /*! \brief Stops the mem-target
     *  \param[in] self          Pointer to this mem-target
     */
    void (*stop)(struct _ocrMemTarget_t* self);

    /**
     * @brief Allocates a chunk of memory for the higher-level
     * allocators to manage
     *
     * @param self          Pointer to this low-memory provider
     * @param size          Size of the chunk to allocate
     * @return Pointer to the chunk of memory allocated
     */
    void* (*allocate)(struct _ocrMemTarget_t* self, u64 size);

    /**
     * @brief Frees a chunk of memory previously allocated
     * by self using allocate
     *
     * @param self          Pointer to this low-memory provider
     * @param addr          Address to free
     */
    void (*free)(struct _ocrMemTarget_t* self, void* addr);
} ocrMemTargetFcts_t;

struct _ocrMemPlatform_t;

/**
 * @brief Target-level memory provider.
 *
 * This represents the target's memories (such as scratchpads)
 */
typedef struct _ocrMemTarget_t {
    ocrMappable_t module; /**< Base "class" for ocrMemTarget */
    ocrGuid_t guid; /**< GUID for this mem-target */
    struct _ocrMemPlatform_t **memories; /**< Pointers to underlying mem-target */
    u64 memoryCount; /**< Number of mem-targets */
    ocrMemTargetFcts_t *fctPtrs; /**< Function pointers for this instance */
} ocrMemTarget_t;


/****************************************************/
/* OCR MEMORY TARGET FACTORY                        */
/****************************************************/
    
/**
 * @brief mem-target factory
 */
typedef struct _ocrMemTargetFactory_t {
    /**
     * @brief Instantiate a new mem-target and returns a pointer to it.
     *
     * @param factory       Pointer to this factory
     * @param instanceArg   Arguments specific for this instance
     */
    ocrMemTarget_t * (*instantiate) (struct _ocrMemTargetFactory_t * factory,
                                     ocrParamList_t* perInstance);
    /**
     * @brief mem-target factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrMemTargetFactory_t * factory);

    ocrMemTargetFcts_t targetFcts; /**< Function pointers created instances should use */
} ocrMemTargetFactory_t;

#endif /* __OCR_MEM_TARGET_H__ */
