/**
 * @brief OCR interface to the datablocks
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_DATABLOCK_H__
#define __OCR_DATABLOCK_H__

#include "ocr-allocator.h"
#include "ocr-types.h"
#include "ocr-utils.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif


/****************************************************/
/* OCR PARAMETER LISTS                              */
/****************************************************/

typedef struct _paramListDataBlockFact_t {
    ocrParamList_t base;    /**< Base class */
} paramListDataBlockFact_t;

typedef struct _paramListDataBlockInst_t {
    ocrParamList_t base;    /**< Base class */
    ocrGuid_t allocator;    /**< Allocator that created this data-block */
    ocrGuid_t allocPD;      /**< Policy-domain of the allocator */
    u64 size;               /**< data-block size */
    void* ptr;              /**< Initial location for the data-block */
    u16 properties;         /**< Properties for the data-block */
} paramListDataBlockInst_t;


/****************************************************/
/* OCR DATABLOCK                                    */
/****************************************************/

struct _ocrDataBlock_t;

typedef struct _ocrDataBlockFcts_t {
    /**
     * @brief Destroys a data-block
     *
     * Again, this does not perform any de-allocation
     * but makes the meta-data invalid and informs
     * the GUID system that the GUID for this data-block
     * is no longer used
     *
     * @param self          Pointer for this data-block
     * @todo: FIXME. This does perform a free!!!!
     */
    void (*destruct)(struct _ocrDataBlock_t *self);

    /**
     * @brief Acquires the data-block for an EDT
     *
     * This call registers a user (the EDT) for the data-block
     *
     * @param self          Pointer for this data-block
     * @param edt           EDT seeking registration
     * @param isInternal    True if this is an acquire implicitly
     *                      done by the runtime at EDT launch
     * @return Address of the data-block
     *
     * @note Multiple acquires for the same EDT have no effect
     */
    void* (*acquire)(struct _ocrDataBlock_t *self, ocrGuid_t edt, bool isInternal);

    /**
     * @brief Releases a data-block previously acquired
     *
     * @param self          Pointer for this data-block
     * @param edt           EDT seeking to de-register from the data-block.
     * @param isInternal    True if matching an internal acquire
     * @return 0 on success and an error code on failure (see ocr-db.h)
     *
     * @note No need to match one-to-one with acquires. One release
     * releases any and all previous acquires
     */
    u8 (*release)(struct _ocrDataBlock_t *self, ocrGuid_t edt, bool isInternal);

    /**
     * @brief Requests that the block be freed when possible
     *
     * This call will return true if the free request was successful.
     * This does not mean that the block was actually freed (other
     * EDTs may be using it), just that the block will be freed
     * at some point
     *
     * @param self          Pointer to this data-block
     * @param edt           EDT seeking to free the data-block
     * @return 0 on success and an error code on failure (see ocr-db.h)
     */
    u8 (*free)(struct _ocrDataBlock_t *self, ocrGuid_t edt);
} ocrDataBlockFcts_t;

/**
 * @brief Internal description of a data-block.
 *
 * This describes the internal representation of
 * a data-block and the meta-data that is associated
 * with it for book-keeping
 **/
typedef struct _ocrDataBlock_t {
    ocrGuid_t guid; /**< The guid of this data-block */
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcess_t statProcess;
#endif
    ocrGuid_t allocator;    /**< Allocator that created this data-block */
    ocrGuid_t allocatorPD;  /**< Policy domain of the creating allocator */
    u64 size;               /**< Size of the data-block */
    void* ptr;              /**< Current location for this data-block */
    u16 properties;         /**< Properties for the data-block */
    ocrDataBlockFcts_t *fctPtrs; /**< Function Pointers for this data-block */
} ocrDataBlock_t;


/****************************************************/
/* OCR DATABLOCK FACTORY                            */
/****************************************************/

/**
 * @brief data-block factory
 */
 typedef struct _ocrDataBlockFactory_t {
    /**
     * @brief Creates a data-block to represent a chunk of memory
     *
     * @param factory       Pointer to this factory
     * @param allocator     Allocator guid used to allocate memory
     * @param allocPD       Policy-domain of the allocator
     * @param size          data-block size
     * @param ptr           Pointer to the memory to use (created through an allocator)
     * @param properties    Properties for the data-block
     * @param instanceArg   Arguments specific for this instance
     **/
    ocrDataBlock_t* (*instantiate)(struct _ocrDataBlockFactory_t *factory,
                                   ocrGuid_t allocator, ocrGuid_t allocatorPD,
                                   u64 size, void* ptr, u16 properties,
                                   ocrParamList_t *instanceArg);
    /**
     * @brief Factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrDataBlockFactory_t *factory);

    ocrDataBlockFcts_t dataBlockFcts; /**< Function pointers created instances should use */
} ocrDataBlockFactory_t;

#endif /* __OCR_DATABLOCK_H__ */
