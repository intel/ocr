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

/**
 * @defgroup OCRTypes Types and constants used in OCR
 *
 * @brief Collection of types and constants used throughout
 * the OCR API
 * @{
 *
 * @defgroup OCRTypesGeneral General types and constants
 *
 * @{
 */
typedef uint64_t u64; /**< 64-bit unsigned integer */
typedef uint32_t u32; /**< 32-bit unsigned integer */
typedef uint16_t u16; /**< 16-bit unsigned integer */
typedef uint8_t  u8;  /**< 8-bit unsigned integer */
typedef int64_t  s64; /**< 64-bit signed integer */
typedef int32_t  s32; /**< 32-bit signed integer */
typedef int8_t   s8;  /**< 8-bit signed integer */

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
 * @}
 *
 * @defgroup OCRTypesDB Types and constants associated with data blocks
 * @{
 */

/**
 * @brief Allocators that can be used to allocate
 * within a data block
 *
 * Data blocks can be used as heaps and allocators can be
 * defined for these heaps. This enum lists the possible
 * allocators.
 */
typedef enum {
    NO_ALLOC = 0 /**< No allocation is possible with the data block */
} ocrInDbAllocator_t;

/**
 * @brief Data block access modes
 *
 * These are the modes with which an EDT can access a data block. OCR currently
 * supports four modes:
 * - Read Only (RO): The EDT is stating that it will only read from the data block.
 *   In this mode, the runtime guarantees that the data block seen by the EDT
 *   is not modified by other concurrent EDTs (in other words, the data block
 *   does not change "under you". Any violation of the "no-write" contract
 *   by the program will result in undefined behavior
 *   (the write may or may not be visible to other EDTs depending on the
 *   implementation and specific runtime conditions).
 * - Non-coherent read (NCR): This mode is exactly the same as RO
 *   except that the runtime does not guarantee that the data block will
 *   not change.
 * - Intent to write (ITW) (default mode): The EDT is stating that it may
 *   or may not write to the data block. The user is responsible for
 *   synchronizing between EDTs that could potentially write to the same
 *   data block concurrently.
 * - Exclusive write (EW): The EDT requires that it be the only one accessing
 *   the data block. The runtime will not schedule any other EDT that accesses
 *   the same data block in EW or ITW mode concurrently.
 *   This can limit parallelism.
 */
typedef enum {
    DB_MODE_RO   = 0x2,   /**< Read-only mode */
    DB_MODE_ITW  = 0x4,   /**< Intent-to-write mode (default mode) */
    DB_MODE_EW   = 0x8,   /**< Exclusive write mode */
    DB_MODE_NCR  = 0x10   /**< Non-coherent read */
} ocrDbAccessMode_t; // Warning: only 5 bits starting at bit 1 (leave bit 0 as 0)

#define DB_ACCESS_MODE_MASK 0x1E /**< Runtime reserved constant */
#define DB_DEFAULT_MODE ((ocrDbAccessMode_t)DB_MODE_ITW) /**< Default access mode */

#define DB_PROP_NONE       ((u16)0x0) /**< Property for a data block indicating no special behavior */
#define DB_PROP_NO_ACQUIRE ((u16)0x10) /**< Property for a data block indicating that the data-block
                                       *   is just being created but does not need to be acquired
                                       *   at the same time (creation for another EDT)
                                       */

#define DB_PROP_SINGLE_ASSIGNMENT ((u16)0x20) /**< Property for a data block indicating single-assignment
                                               *   i.e. The user guarantees the data block is written once
                                               *   at creation time.
                                               *   @note This property is experimental and not
                                               *   implemented consistently.
                                               */

/**
 * @}
 *
 * @defgroup OCRTypesEDT Types and constants associated with EDTs
 *
 * @{
 */

/**
 * @brief Type of values passed to an EDT on each pre-slot
 *
 * An EDT with N pre-slots will receive an array of N elements of this
 * type (its input dependences). Each dependence has the GUID of the
 * data block passed along that pre-slot as well as a pointer
 * to the data in the data block.
 *
 * @note The GUID passed to the EDT is *not* the GUID of the event
 * linked to the pre-slot of the EDT but rather the GUID of the data block
 * that was associated with that event. If no data block was associated, NULL_GUID
 * is passed.
 */
typedef struct {
    ocrGuid_t guid; /**< GUID of the data block or NULL_GUID */
    void* ptr;      /**< Pointer allowing access to the data block or NULL */
} ocrEdtDep_t;


#define EDT_PROP_NONE   ((u16) 0x0) /**< Property bits indicating a regular EDT */
#define EDT_PROP_FINISH ((u16) 0x1) /**< Property bits indicating a FINISH EDT */

/**
 * @brief Constant indicating that the number of parameters or dependences
 * to an EDT or EDT template is unknown
 *
 * An EDT is created as an instance of an EDT template. The number of
 * parameters or dependences for the EDT can either be specified when
 * creating the template or when creating the EDT. This constant indicates
 * that the number of parameters or dependences is still unknown (for example,
 * when creating the template).
 *
 * When the EDT is created, the number of parameters and dependences must
 * be known (either specified in the template or the EDT). In other words, you
 * cannot specify the number of parameters or dependences to be EDT_PARAM_UNK
 * in both the creation of the template and the EDT.
 */
#define EDT_PARAM_UNK   ((u32)-1)

/**
 * @brief Constant indicating that the number of parameters or
 * dependences to an EDT is the same as the one specified in its template.
 */
#define EDT_PARAM_DEF   ((u32)-2)

/**
 * @brief Type for an EDT
 *
 * This is the function prototype for all EDTs.
 * @param[in] paramc   Number of non-data block parameters. A parameter is a 64-bit
 *                     value known at the creation time of the EDT
 * @param[in] paramv   Values of the 'paramc' parameters
 * @param[in] depc     Number of dependences. This corresponds to the number of
 *                     pre-slots for the EDT
 * @param[in] depv     GUIDs and pointers to the data blocks passed to this
 *                     EDT on its pre-slots. The GUID may be NULL_GUID if the pre-slot
 *                     was a pure control dependence.
 * @return The GUID of a data block to pass along to the pre-slot of the output
 * event optionally associated with this EDT. NULL_GUID can also be returned.
 **/
typedef ocrGuid_t (*ocrEdt_t)(u32 paramc, u64* paramv,
                              u32 depc, ocrEdtDep_t depv[]);

/**
 * @}
 *
 * @defgroup OCRTypesEvents Types and constants associated with events
 *
 * @{
 */

/**
 * @brief Types of OCR events
 *
 * Each OCR event has a type that is specified at creation.
 * The type of the event determines its behavior, specifically:
 * - its persistency after it triggers its post-slot
 * - its trigger rule
 * - its behavior when satisfied multiple times
 */
typedef enum {
    OCR_EVENT_ONCE_T,    /**< A ONCE event simply passes along a satisfaction on its
                          * unique pre-slot to its post-slot. Once all OCR objects
                          * linked to its post-slot have been satisfied, the ONCE event
                          * is automatically destroyed. */
    OCR_EVENT_IDEM_T,    /**< An IDEM event simply passes along a satisfaction on its
                          * unique pre-slot to its post-slot. The IDEM event persists
                          * until ocrEventDestroy() is explicitly called on it.
                          * It can only be satisfied once and susequent
                          * satisfactions are ignored (use case: BFS, B&B..) */
    OCR_EVENT_STICKY_T,  /**< A STICKY event is identical to an IDEM event except that
                          * multiple satisfactions result in an error
                          */
    OCR_EVENT_LATCH_T,   /**< A LATCH event has two pre-slots: a INCR and a DECR.
                          * Each slot is associated with an internal monotonically
                          * increasing counter that starts at 0. On each satisfaction
                          * of one of the pre-slots, the counter for that slot is
                          * incremented by 1. When both counters are equal (and non-zero),
                          * the post-slot of the latch event is triggered.
                          * Any data block passed along its pre-slots is ignored.
                          * A LATCH event has the same persistent as a ONCE event and
                          * is automatically destroyed when its post-slot is triggered.
                          */
    OCR_EVENT_T_MAX      /**< This is *NOT* an event and is only used to count
                          * the number of event types. Its use is reserved for the
                          * runtime. */
} ocrEventTypes_t;


/**
 * @brief Pre-slots for events
 *
 * Currently, only the LATCH event has more than one pre-slot.
 */
typedef enum {
    OCR_EVENT_LATCH_DECR_SLOT = 0, /**< The decrement slot of a LATCH event */
    OCR_EVENT_LATCH_INCR_SLOT = 1  /**< The increment slot of a LATCH event */
} ocrLatchEventSlot_t;

/**
 * @}
 * @}
 */


#endif /* __OCR_TYPES_H__ */


