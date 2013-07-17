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
typedef struct _paramListMemTargetFact_t {
    ocrParamList_t base;
} paramListMemTargetFact_t;

typedef struct _paramListMemTargetInst_t {
    ocrParamList_t base;
} paramListMemTargetInst_t;


/****************************************************/
/* OCR MEMORY TARGETS                               */
/****************************************************/

struct _ocrMemTarget_t;
struct _ocrPolicyDomain_t;

typedef struct _ocrMemTargetFcts_t {
    /**
     * @brief Destructor equivalent
     *
     * @param self          Pointer to this low-memory provider
     */
    void (*destruct)(struct _ocrMemTarget_t* self);

    void (*start)(struct _ocrMemTarget_t* self, struct _ocrPolicyDomain_t * PD);

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
 *
 */
typedef struct _ocrMemTarget_t {
    ocrMappable_t module; /**< Base "class" for ocrMemTarget */
    ocrGuid_t guid;

    struct _ocrMemPlatform_t **memories;
    u64 memoryCount;

    ocrMemTargetFcts_t *fctPtrs;
} ocrMemTarget_t;

/****************************************************/
/* OCR MEMORY TARGET FACTORY                        */
/****************************************************/

typedef struct _ocrMemTargetFactory_t {
    ocrMemTarget_t * (*instantiate) (struct _ocrMemTargetFactory_t * factory,
                                     ocrParamList_t* perInstance);
    void (*destruct)(struct _ocrMemTargetFactory_t * factory);

    ocrMemTargetFcts_t targetFcts;
} ocrMemTargetFactory_t;

#endif /* __OCR_MEM_TARGET_H__ */
