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

#include "ocr-runtime-types.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif


struct _ocrPolicyDomain_t;

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

/**
 * @brief mem-target function pointers
 *
 * The function pointers are separate from the mem-target instance to allow for
 * the sharing of function pointers for mem-target from the same factory
 */
typedef struct _ocrMemTargetFcts_t {
    /** @brief Destroys a mem-target
     *  @param[in] self          Pointer to this mem-target
     */
    void (*destruct)(struct _ocrMemTarget_t* self);

    /** @brief Starts the mem-target
     *  @param[in] self          Pointer to this mem-target
     *  @param[in] PD            Current policy domain
     */
    void (*start)(struct _ocrMemTarget_t* self, struct _ocrPolicyDomain_t * PD);

    /** @brief Stops the mem-target
     *  @param[in] self          Pointer to this mem-target
     */
    void (*stop)(struct _ocrMemTarget_t* self);

    void (*finish)(struct _ocrMemTarget_t* self);

    /**
     * @brief Gets the throttle value for this memory
     *
     * A value of 100 indicates nominal throttling.
     *
     * @param[in] self        Pointer to this mem-target
     * @param[out] value      Throttling value
     * @return 0 on success or the following error code:
     *     - 1 if the functionality is not supported
     *     - other codes implementation dependent
     */
    u8 (*getThrottle)(struct _ocrMemTarget_t* self, u64 *value);

    /**
     * @brief Sets the throttle value for this memory
     *
     * A value of 100 indicates nominal throttling.
     *
     * @param[in] self        Pointer to this mem-target
     * @param[in] value       Throttling value
     * @return 0 on success or the following error code:
     *     - 1 if the functionality is not supported
     *     - other codes implementation dependent
     */
    u8 (*setThrottle)(struct _ocrMemTarget_t* self, u64 value);

    /**
     * @brief Gets the start and end address (a u64 value) of the
     * region of memory
     *
     * @param[in] self        Pointer to this mem-target
     * @param[out] startAddr  Start address of the memory region
     * @param[out] endAddr    End address of the memory region
     */
    void (*getRange)(struct _ocrMemTarget_t* self, u64* startAddr,
                     u64* endAddr);

    /**
     * @brief Requests a chunk of contiguous memory of size 'size' and of
     * type 'desiredTag'
     *
     * @param self         Pointer to this mem-target
     * @param startAddr    Return value: start address of the chunk
     * @param size         Size requested (in bytes)
     * @param oldTag       Current tag for the chunk
     * @param newTag       Tag any successfully returned chunk with
     *                     this new tag
     *
     * @return 0 on success and the following codes on error:
     *     - 1 if no chunk with the desired tag exists
     *     - 2 if a chunk with the desired tag exists but none are big enough to accomodate size
     *     - 3 if size/desiredTag are invalid
     *     - other non-zero codes implementation dependent
     * @note This operation is atomic with respect to other chunkAndTag
     * and tag calls
     */
    u8 (*chunkAndTag)(struct _ocrMemTarget_t* self, u64 *startAddr, u64 size,
                      ocrMemoryTag_t oldTag, ocrMemoryTag_t newTag);

    /**
     * @brief Tag a region of memory with a specific tag.
     *
     * @param self         Pointer to this mem-target
     * @param startAddr    Start address to tag (included)
     * @param endAddr      End address to tag (included)
     * @param newTag       Tag to set for these addresses
     *
     * @return 0 on success and a non-zero code on error 
     * @note There is no forced check as to whether the entire region
     * being tagged is of the same old tag (ie: a uniform region); that
     * is up to the implementation
     */
    u8 (*tag)(struct _ocrMemTarget_t *self, u64 startAddr, u64 endAddr,
              ocrMemoryTag_t newTag);

    /**
     * @brief Return the tag of an address in memory as well as the
     * start and end of the largest region having the same tag as addr
     *
     * This call will query for the tag of an address and return the
     * largest region for which the tag is the same around that address
     *
     * @param self         Pointer to this mem-target
     * @param startAddr    Return value: start of the range with the same tag
     * @param endAddr      Return value: end of the range with the same tag
     * @param resultTag    Return value: tag of the address
     * @param addr         Address to query
     *
     * @return 0 on success and the following error codes:
     *     - 1 if address is out of range
     *     - other codes implementation dependent
     */
    u8 (*queryTag)(struct _ocrMemTarget_t *self,
                   u64 *start, u64 *end, ocrMemoryTag_t *resultTag,
                   u64 addr);
} ocrMemTargetFcts_t;

struct _ocrMemPlatform_t;

/**
 * @brief Target-level memory provider.
 *
 * This represents the target's memories (such as scratchpads)
 */
typedef struct _ocrMemTarget_t {
    ocrFatGuid_t fguid; /**< GUID for this mem-target */
    struct _ocrPolicyDomain_t *pd; /**< Policy domain that uses this mem-target */
    ocrPhysicalLocation_t location;
    u64 size, startAddr, endAddr;
    struct _ocrMemPlatform_t **memories; /**< Pointers to underlying mem-target */
    u64 memoryCount;                     /**< Number of mem-targets */
    ocrMemTargetFcts_t fcts;             /**< Functions for this instance */
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
                                     ocrPhysicalLocation_t location,
                                     u64 memSize, ocrParamList_t* perInstance);
    /**
     * @brief mem-target factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrMemTargetFactory_t * factory);

    ocrMemTargetFcts_t targetFcts; /**< Function pointers created instances should use */
} ocrMemTargetFactory_t;

#endif /* __OCR_MEM_TARGET_H__ */
