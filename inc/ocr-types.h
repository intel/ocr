/**
 * @brief Basic types used throughout the OCR library
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_TYPES_H__
#define __OCR_TYPES_H__

#include <stddef.h>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int64_t  s64;
typedef int32_t  s32;
typedef int8_t   s8;

/* boolean support in C */
#ifndef __cplusplus
#define true 1
#define TRUE 1
#define false 0
#define FALSE 0
typedef u8 bool;
#endif /* __cplusplus */

#ifdef __cplusplus
#define TRUE true
#define FALSE false
#endif /* __cplusplus */

/**
 * @brief Type describing the unique identifier of most
 * objects in OCR (EDTs, data-blocks, etc).
 **/
typedef intptr_t ocrGuid_t; /**< GUID type */
//typedef u64 ocrGuid_t;

/**
 * @brief A NULL ocrGuid_t
 */
#define NULL_GUID ((ocrGuid_t)0x0)

/**
 * @brief An Unitialized GUID (ie: never set)
 */
#define UNINITIALIZED_GUID ((ocrGuid_t)-2)

/**
 * @brief An invalid GUID
 */
#define ERROR_GUID ((ocrGuid_t)-1)


/**
 * @brief Allocators that can be used to allocate
 * within a data-block
 */
typedef enum {
    NO_ALLOC = 0 /**< No allocation inside data-blocks */
               /* Add others */
} ocrInDbAllocator_t;

/**
 * @brief Types of events supported
 *
 * @todo Re-evaluate the notion of bucket/multiple-satisfaction events (post 0.7)
 */
typedef enum {
    OCR_EVENT_ONCE_T,    /**< The event is automatically destroyed on satisfaction */
    OCR_EVENT_IDEM_T,    /**< The event exists until explicitly destroyed with
                          * ocrEventDestroy(). It is satisfied once and susequent
                          * satisfactions are ignored (use case: BFS, B&B..) */
    OCR_EVENT_STICKY_T,  /**< The event exists until explicitly destroyed with
                          * ocrEventDestroy(). Multiple satisfactions result
                          * in an error */
    OCR_EVENT_LATCH_T,   /**< The latch event can be satisfied on either
                          * its the DECR or INCR slot. When it reaches zero,
                          * it is satisfied. It behaves like a ONCE event
                          * in the sense that it is destroyed on satisfaction
                          */
    OCR_EVENT_T_MAX      /**< Marker */
} ocrEventTypes_t;


/**
 * @brief "Slots" for latch events
 *
 * A latch event will be satisfied when it has received a number of
 * "satisfactions" on its DECR_SLOT equal to 1 + the number of "satisfactions"
 * on its INCR_SLOT
 */
typedef enum {
    OCR_EVENT_LATCH_DECR_SLOT = 0, /**< The decrement slot */
    OCR_EVENT_LATCH_INCR_SLOT = 1  /**< The increment slot */
} ocrLatchEventSlot_t;

/**
 * @brief Type of the input passed to each EDT
 *
 * This struct associates a DB GUID with its base pointer (implicitly acquired)
 */
typedef struct {
    ocrGuid_t guid;
    void* ptr;
} ocrEdtDep_t;


#define EDT_PROP_NONE   ((u16) 0x0) /**< Property bits indicating a regular EDT */
#define EDT_PROP_FINISH ((u16) 0x1) /**< Property bits indicating a FINISH EDT */

/**
 * @brief Constant indicating that the number of parameters to an EDT template
 * is unknown
 */
#define EDT_PARAM_UNK   ((u32)-1)

/**
 * @brief Constant indicating that the number of parameters to an EDT
 * is the same as the one specified in its template
 */
#define EDT_PARAM_DEF   ((u32)-2)

/**
 * @brief Type for an EDT
 *
 * @param paramc          Number of non-DB or non-event parameters. Can be used to pass u64 values
 * @param paramv          Values for the u64 values
 * @param depc            Number of dependences (either DBs or events)
 * @param depv            Values of the dependences. Can be NULL_GUID if a pure control-flow event
 *                        was used as a dependence
 * @return The GUID of a data-block to pass along on the output event of
 * the EDT (or NULL_GUID)
 **/
typedef ocrGuid_t (*ocrEdt_t)(u32 paramc, u64* paramv,
                              u32 depc, ocrEdtDep_t depv[]);
/**
 * @brief Data-block access modes
 *
 * These are the modes with which an EDT can access a DB. We currently support
 * three modes:
 *     - Read Only: The EDT is stating that it will only read from the DB.
 *       This enables the runtime to provide a copy for example with no need
 *       to "merge" things back together as there is no write. Note that
 *       any writes to the DB will potentially be disgarded
 *     - Intent to write (default mode): The EDT is possibly going to write
 *       to the DB but does not require an exclusive access to it. The user
 *       is responsible for synchronizing between EDTs that can potentially
 *       write to the same DB at the same time.
 *     - Exclusive write: The EDT requires that it be the only one accessing
 *       the DB. The runtime will not schedule any other EDT that accesses
 *       the same DB while the EDT with an exclusive write access is running.
 *       This potentially limites parallelism
 */
typedef enum {
    DB_MODE_RO   = 0x2,   /**< Read-only mode */
    DB_MODE_ITW  = 0x4,   /**< Intent-to-write mode (default mode) */
    DB_MODE_EW   = 0x8,   /**< Exclusive write mode */
    DB_MODE_NCR  = 0x10   /**< Non-coherent read: like Read-only but the copy may change from under you */
} ocrDbAccessMode_t; // Warning: only 5 bits starting at bit 1 (leave bit 0 as 0)

#define DB_ACCESS_MODE_MASK 0x1E
#define DB_DEFAULT_MODE ((ocrDbAccessMode_t)DB_MODE_ITW)

#define DB_PROP_NONE       ((u16)0x0) /**< Property for a data-block indicating no special behavior */
#define DB_PROP_NO_ACQUIRE ((u16)0x10) /**< Property for a data-block indicating that the data-block
                                       *   is just being created but does not need to be acquired
                                       *   at the same time (creation for another EDT)
                                       */

#define DB_PROP_SINGLE_ASSIGNMENT ((u16)0x20) /**< Property for a data-block indicating single-assignment
                                               *   i.e. The user guarantees the datablock is written once
                                               *   at creation time.
                                               */
#endif /* __OCR_TYPES_H__ */


