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

#include "ocr-types.h"
#include "ocr-utils.h"


/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

/**
 * @brief Parameter list to create a mem-platform factory
 */
typedef struct _paramListMemPlatformFact_t {
    ocrParamList_t base;
} paramListMemPlatformFact_t;

/**
 * @brief Parameter list to create a mem-platform instance
 */
typedef struct _paramListMemPlatformInst_t {
    ocrParamList_t base;
} paramListMemPlatformInst_t;


/****************************************************/
/* OCR MEMORY PLATFORM                              */
/****************************************************/

struct _ocrMemPlatform_t;
struct _ocrPolicyDomain_t;

/**
 * @brief mem-platform function pointers
 *
 * The function pointers are separate from the mem-platform instance to allow for
 * the sharing of function pointers for mem-platform from the same factory
 */
typedef struct _ocrMemPlatformFcts_t {
    /** @brief Destroys a mem-platform
     *  @param[in] self       Pointer to this mem-platform
     */
    void (*destruct)(struct _ocrMemPlatform_t* self);

    /** @brief Starts the mem-platform
     *
     *  @param[in] self       Pointer to this mem-platform
     *  @param[in] PD         Current policy domain
     */
    void (*start)(struct _ocrMemPlatform_t* self, struct _ocrPolicyDomain_t * PD);

    /** @brief Stops the mem-platform
     *  @param[in] self       Pointer to this mem-platform
     */
    void (*stop)(struct _ocrMemPlatform_t* self);

    /**
     * @brief Gets the throttle value for this memory
     *
     * A value of 100 indicates nominal throttling.
     *
     * @param[in] self        Pointer to this mem-platform
     * @param[out] value      Throttling value
     * @return 0 on success or the following error code:
     *     - 1 if the functionality is not supported
     *     - other codes implementation dependent
     */
    u8 (*getThrottle)(struct _ocrMemPlatform_t* self, u64 *value);

    /**
     * @brief Sets the throttle value for this memory
     *
     * A value of 100 indicates nominal throttling.
     *
     * @param[in] self        Pointer to this mem-platform
     * @param[in] value       Throttling value
     * @return 0 on success or the following error code:
     *     - 1 if the functionality is not supported
     *     - other codes implementation dependent
     */
    u8 (*setThrottle)(struct _ocrMemPlatform_t* self, u64 value);

    /**
     * @brief Gets the start and end address (a u64 value) of the
     * region of memory
     *
     * @param[in] self        Pointer to this mem-platform
     * @param[out] startAddr  Start address of the memory region
     * @param[out] endAddr    End address of the memory region
     */
    void (*getRange)(struct _ocrMemPlatform_t* self, u64* startAddr,
                     u64* endAddr);

    /**
     * @brief Requests a chunk of contiguous memory of size 'size' and of
     * type 'desiredTag'
     *
     * @param self         Pointer to this mem-platform
     * @param startAddr    Return value: start address of the chunk
     * @param size         Size requested (in bytes)
     * @param oldTag       Current tag for the chunk
     * @param newTag       Tag any successfully returned chunk with
     *                     this tag
     *
     * @return 0 on success and the following codes on error:
     *     - 1 if a chunk with the desired tag exists but none are big enough to accomodate size
     *     - 2 if no chunk with the desired tag exists
     *     - 3 if size/desiredTag are invalid
     *     - other non-zero codes implementation dependent
     * @note This operation is atomic with respect to other chunkAndTag
     * and tag calls
     */
    u8 (*chunkAndTag)(struct _ocrMemPlatform_t* self, u64 *startAddr, u64 size,
                      ocrMemoryTag_t oldTag, ocrMemoryTag_t newTag);

    /**
     * @brief Tag a region of memory with a specific tag.
     *
     * @param self         Pointer to this mem-platform
     * @param startAddr    Start address to tag (included)
     * @param endAddr      End address to tag (included)
     * @param newTag       Tag to set for these addresses
     *
     * @return 0 on success and a non-zero code on error 
     * @note There is no forced check as to whether the entire region
     * being tagged is of the same old tag (ie: a uniform region); that
     * is up to the implementation
     */
    u8 (*tag)(struct _ocrMemPlatform_t *self, u64 startAddr, u64 endAddr,
              ocrMemoryTag_t newTag);

    /**
     * @brief Return the tag of an address in memory as well as the
     * start and end of the largest region having the same tag as addr
     *
     * This call will query for the tag of an address and return the
     * largest region for which the tag is the same around that address
     *
     * @param self         Pointer to this mem-platform
     * @param startAddr    Return value: start of the range with the same tag
     * @param endAddr      Return value: end of the range with the same tag
     * @param resultTag    Return value: tag of the address
     * @param addr         Address to query
     *
     * @return 0 on success and the following error codes:
     *     - 1 if address is out of range
     *     - other codes implementation dependent
     */
    u8 (*queryTag)(struct _ocrMemPlatform_t *self,
                   u64 *start, u64 *end, ocrMemoryTag_t *resultTag,
                   u64 addr);
} ocrMemPlatformFcts_t;

/**
 * @brief Memory-platform
 *
 * This abstracts the platform's memory resources which must be
 * of a fixed size.
 */
typedef struct _ocrMemPlatform_t {
    u64 size, startAddr, endAddr;  /**< Size, start and end address for this instance */
    ocrMemPlatformFcts_t *fctPtrs; /**< Function pointers for this instance */
} ocrMemPlatform_t;


/****************************************************/
/* OCR MEMORY PLATFORM FACTORY                      */
/****************************************************/

/**
 * @brief mem-platform factory
 */

typedef struct _ocrMemPlatformFactory_t {
    /**
     * @brief Instantiate a new mem-platform and returns a pointer to it.
     *
     * @param factory[in]     Pointer to this factory
     * @param memSize[in]     Size of the underlying memory
     * @param instanceArg[in] Arguments specific for this instance
     */
    ocrMemPlatform_t * (*instantiate) (struct _ocrMemPlatformFactory_t * factory,
                                       u64 memSize, ocrParamList_t* instanceArg);
    /**
     * @brief mem-platform factory destructor
     * @param factory[in]     Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrMemPlatformFactory_t * factory);

    ocrMemPlatformFcts_t platformFcts; /**< Function pointers created instances should use */
} ocrMemPlatformFactory_t;

#endif /* __OCR_MEM_PLATFORM_H__ */
