/**
 * @brief OCR interface to the low level memory interface
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */



#ifndef __OCR_MEM_PLATFORM_H__
#define __OCR_MEM_PLATFORM_H__

#include "ocr-mappable.h"
#include "ocr-types.h"
#include "ocr-utils.h"

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/
typedef struct _paramListMemPlatformFact_t {
    ocrParamList_t base;
} paramListMemPlatformFact_t;

typedef struct _paramListMemPlatformInst_t {
    ocrParamList_t base;
} paramListMemPlatformInst_t;


/****************************************************/
/* OCR MEMORY PLATFORM                              */
/****************************************************/

struct _ocrMemPlatform_t;
struct _ocrPolicyDomain_t;

typedef struct _ocrMemPlatformFcts_t {
    /**
     * @brief Destructor equivalent
     *
     * @param self          Pointer to this low-memory provider
     */
    void (*destruct)(struct _ocrMemPlatform_t* self);

    void (*start)(struct _ocrMemPlatform_t* self, struct _ocrPolicyDomain_t * PD);

    void (*stop)(struct _ocrMemPlatform_t* self);

    /**
     * @brief Allocates a chunk of memory for the higher-level
     * allocators to manage
     *
     * @param self          Pointer to this low-memory provider
     * @param size          Size of the chunk to allocate
     * @return Pointer to the chunk of memory allocated
     */
    void* (*allocate)(struct _ocrMemPlatform_t* self, u64 size);

    /**
     * @brief Frees a chunk of memory previously allocated
     * by self using allocate
     *
     * @param self          Pointer to this low-memory provider
     * @param addr          Address to free
     */
    void (*free)(struct _ocrMemPlatform_t* self, void* addr);
} ocrMemPlatformFcts_t;

/**
 * @brief Low-level memory provider.
 *
 * This allows low-level memory allocation (such as malloc)
 *
 * @warning Currently only uses malloc with no consideration for the
 * underlying machine but this will change as support for distributed and/or NUMA
 * architecture comes online. The API may therefore evolve
 */
typedef struct _ocrMemPlatform_t {
    ocrMappable_t module; /**< Base "class" for ocrMemPlatform */

    ocrMemPlatformFcts_t *fctPtrs;
} ocrMemPlatform_t;

/****************************************************/
/* OCR MEMORY PLATFORM FACTORY                      */
/****************************************************/
typedef struct _ocrMemPlatformFactory_t {
    ocrMemPlatform_t * (*instantiate) (struct _ocrMemPlatformFactory_t * factory,
                                       ocrParamList_t* perInstance);
    void (*destruct)(struct _ocrMemPlatformFactory_t * factory);

    ocrMemPlatformFcts_t platformFcts;
} ocrMemPlatformFactory_t;
#endif /* __OCR_MEM_PLATFORM_H__ */
