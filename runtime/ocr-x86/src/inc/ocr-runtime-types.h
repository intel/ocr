/**
 * @brief Basic types used throughout the OCR library (runtime only types)
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_RUNTIME_TYPES_H__
#define __OCR_RUNTIME_TYPES_H__

#include "ocr-types.h"

/**
 * @brief Memory region "tags"
 *
 * A tag indicates the type of the memory region in the
 * mem-target or mem-platform. Other tags may be added
 * as needed.
 * @see chunk() tag() queryTag()
 * @note Must be sequential and have MAX_TAG for rangeTracker to work
 */
typedef enum {
    RESERVED_TAG   = 0, /**< Non-usable chunk of memory */
    NON_USER_TAG    = 1, /**< Regions that are non user managed (text, bss for example */
    USER_FREE_TAG   = 2, /**< Regions that are free to be used by higher levels */
    USER_USED_TAG   = 3, /**< Regions that the higher levels have used */
    MAX_TAG         = 4  /**< Marker of the end of the tags */
} ocrMemoryTag_t;

/**
 * @brief "Capabilities" for comp-target and comp-platform
 *
 * Computation resources can have capabilities that allow minimal
 * support for heterogeneous infrastructure.
 */
typedef enum {
    COMP_CAPABILITY = 0x1, /**< Node is capable of computation */
    COMM_CAPABILITY = 0x2, /**< Node is capable of communicating with other nodes */
    MAX_CAPABILITY  = 0x3  /**< OR of all the previous capabilities */
} ocrComputeCapability_t;

/**
 * @brief Types of data-blocks allocated by the runtime
 *
 * OCR provides support for "runtime" data-blocks which
 * may have more restrictive rules than regular data-blocks.
 *
 * @warn This functionality is not currently used and may be
 * stripped out but it is here in case we need it later
 */
typedef enum {
    USER_DBTYPE     = 0x1,
    RUNTIME_DBTYPE  = 0x2,
    MAX_DBTYPE      = 0x3
} ocrDataBlockType_t;

/**
 * @brief Types of work "scheduled" by the runtime
 *
 * For now this is EDT but other type of computation
 * may be used (runtime EDTs, communication tasks, etc)
 *
 * @warn This functionality is not currently used and may be
 * stripped out but it is here in case we need it later
 */
typedef enum {
    EDT_WORKTYPE    = 0x1,
    MAX_WORKTYPE    = 0x3
} ocrWorkType_t;

typedef struct {
    ocrGuid_t guid;
    void* metaDataPtr;
} ocrFatGuid_t;

//TODO: FIXME: Placeholder for an ID for target/platform
//compute or memories
typedef u64 ocrPhysicalLocation_t;

/**
 * @brief Returned by the pollMessage function in
 * either the comp-target and comp-platform to indicate
 * that no message was available */
#define POLL_NO_MESSAGE   0x1
/**
 * @brief Indicates that a message was returned and available
 * and that more messages are available
 */
#define POLL_MORE_MESSAGE 0x2
/**
 * @brief AND the return code of pollMessage with this
 * mask to get any real error codes
 */
#define POLL_ERR_MASK     0xF0

#endif /* __OCR_RUNTIME_TYPES_H__ */



