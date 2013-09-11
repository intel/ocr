/**
 * @brief Event Driven Tasks, events and dependences
 * API
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_EDT_H__
#define __OCR_EDT_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "ocr-types.h"

/**
   @defgroup OCREvents Event Management for OCR
   @brief Describes the event management APIs for OCR

   @{
**/

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
                          * it is satisfied. */
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
 * @brief Creates an event
 *
 * This function creates a new programmer-managed event identified by the
 *  returned GUID.
 *
 * @param guid            The new GUID allocated for the new event
 * @param eventType       The type of the event to create
 * @param takesArg        True if this event will carry data with it or false otherwise
 * @return 0 on success and an error code on failure:
 *     - ENOMEM: Returned if space cannot be found to allocate the event
 *     - EINVAL: Returned if event_type was malformed or is incompatible with takesArg
 **/
u8 ocrEventCreate(ocrGuid_t *guid, ocrEventTypes_t eventType, bool takesArg);

/**
 * @brief Explicitly destroys an event
 *
 * Certain events, such as sticky and idempotent events need to
 * be destroyed explicitly because they are not destroyed when satisfied
 *
 * @param guid            The GUID of the event to delete
 * @return 0 on success and an error code on failure:
 *     - EINVAL: Returned if the GUID pass does not refer to a valid event to destroy
 **/
u8 ocrEventDestroy(ocrGuid_t guid);

/**
 * @brief Satisfy an event and optionally pass a data-block along with the event
 *
 * An 'event' encodes a control dependence while the data-block optionally passed
 * with the even encodes a data dependence. Therefore:
 *     - An event without a data-block is a pure control dependence
 *     - An event with a data-block is a control+data dependence
 *     - A pure data-dependence can be encoded by directly adding a dependence
 *       using ocrAddDependence with a data-block as the source
 *
 * @param eventGuid       GUID of event to satisfy
 * @param dataGuid        GUID of data to pass along or NULL_GUID if no data
 * @return 0 on success and an error code on failure:
 *     - ENOMEM: Returned if there is not enough memory. This is usually caused by
 *               a programmer error (two events to a unique slot and both events are
 *               satisfied)
 *     - EINVAL: If the GUIDs do not refer to valid events/data-blocks
 *     - ENOPERM: If the event has already been satisfied or if the event does not take
 *                an argument and one is given or if the event takes an argument and none
 *                is given
 *
 * An event satisfaction without the optional data-block can be viewed as a pure
 * control dependence whereas one with a data-block is a control+data dependence
 *
 * @note On satisfaction, a OCR_EVENT_ONCE_T event will pass the GUID of the
 * optional attached data-block to all EDTs/Events waiting on it at that time
 * and the event will destroy itself. For OCR_EVENT_IDEM_T and OCR_EVENT_STICKY_T,
 * the event will be satisfied for all EDTs/Events currently waiting on it
 * as well as any future EDT/Event that adds it as a dependence until it is destroyed
 **/
u8 ocrEventSatisfy(ocrGuid_t eventGuid, ocrGuid_t dataGuid /*=NULL_GUID*/);

/**
 * @brief Similar to ocrEventSatisfy() but with the slot information
 *
 * This call is used primarily for latch events.
 * ocrEventSatisfySlot(eventGuid, dataGuid, 0) is equivalent to
 * ocrEventSatisfy(eventGuid, dataGuid)
 *
 * @see ocrEventSatisfy()
 */
u8 ocrEventSatisfySlot(ocrGuid_t eventGuid, ocrGuid_t dataGuid /*=NULL_GUID*/, u32 slot /*=0*/);

/**
   @}
**/

/**
 * @defgroup OCREDT Event Driven Task Management
 * @brief APIs to manage the EDT in OCR
 *
 * @{
 **/

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
typedef ocrGuid_t (*ocrEdt_t )( u32 paramc, u64* paramv,
                   u32 depc, ocrEdtDep_t depv[]);


/**
 * @brief Creates an EDT template
 *
 * An EDT template encapsulates the code that will be executed
 * by the EDT. It needs to be created only once for each function that will serve
 * as an EDT.
 *
 * @param guid              Returned value: GUID of the newly created EDT Template
 * @param funcPtr           Function to execute as the EDT
 * @param paramc            Number of u64 non-DB values or EDT_PARAM_UNK
 * @param depc              Number of data-blocks or EDT_PARAM_UNK
 *
 * @return 0 on success and an error code on failure: TODO
 */
u8 ocrEdtTemplateCreate(ocrGuid_t *guid, ocrEdt_t funcPtr, u32 paramc, u32 depc);

/**
 * @brief Destroy an EDT template
 *
 * This function can be called if no further EDTs will be created based on this
 * template
 *
 * @param guid              GUID of the EDT template to destroy
 * @return 0 if successful
 */
u8 ocrEdtTemplateDestroy(ocrGuid_t guid);

/**
 * @brief Creates an EDT instance
 *
 * @param guid              Returned value: GUID of the newly created EDT type
 * @param templateGuid      GUID of the template to use for this EDT
 * @param paramc            Number of non-DB 64 bit values. Set to 'EDT_PARAM_DEF' if
 *                          follows the 'paramc' template specification. Set to the
 *                          actual number of arguments to be passed if 'EDT_PARAM_UNK'
 *                          has been given as the template's 'paramc' value.
 * @param paramv            Values for those parameters (copied in)
 * @param properties        Used to indicate if this is a finish EDT (EDT_PROP_FINISH).
 *                          Other uses reserved.
 * @param affinity          Affinity container for this EDT. Can be NULL_GUID
 * @param depc              Number of dependences for this EDT. Set to 'EDT_PARAM_DEF' if
 *                          it follows the 'depc' template specification. Set to the actual
 *                          number of arguments to be passed if 'EDT_PARAM_UNK' has been
 *                          given as the template's 'depc' value.
 * @param depv              Values for the GUIDs of the dependences (if known)
 *                          Use ocrAddDependence to add unknown ones or ones with
 *                          a mode other than the default DB_MODE_ITW
 * @param outputEvent       Returned value: If not NULL, will return the GUID
 *                          of the event that will be satisfied when this EDT
 *                          completes. In the case of a finish EDT, this event
 *                          will be satisfied *only* if all transitively created
 *                          EDTs inside this EDT have also completed. If NULL,
 *                          no output event will be created or satisfied
 * @return 0 on success and an error code on failure: TODO
 **/
u8 ocrEdtCreate(ocrGuid_t * guid, ocrGuid_t templateGuid,
                u32 paramc, u64* paramv, u32 depc, ocrGuid_t *depv,
                u16 properties, ocrGuid_t affinity, ocrGuid_t *outputEvent);

/**
 * @brief Destroy an EDT
 *
 * This should be called if it becomes known that the EDT
 * will never be able to run
 *
 * @param guid              GUID of the EDT to destroy
 * @return Status:
 *      - 0: Successful
 */
u8 ocrEdtDestroy(ocrGuid_t guid);

/**
   @}
**/


/**
   @defgroup OCRDependences OCR Dependences
   @brief API to manage OCR dependences

   @{
**/

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
    DB_MODE_RO,     /**< Read-only mode */
    DB_MODE_ITW,    /**< Intent-to-write mode (default mode) */
    DB_MODE_EW      /**< Exclusive write mode */
} ocrDbAccessMode_t;

#define DB_DEFAULT_MODE (ocrDbAccessMode_t)DB_MODE_ITW
/**
 * @brief Adds a dependence between entities in OCR:
 *
 * The following dependences can be added:
 *    - Event to Event: The sink event will be satisfied when
 *                      the source event is
 *    - Event to EDT  : Control or control+data dependence
 *    - DB to Event   : Equivalent to ocrEventSatisfySlot
 *    - DB to EDT     : Pure data dependence. Semantically
 *                      equivalent to giving a sticky event
 *                      pre-satisfied with the DB
 *
 * @param source            GUID of the source
 * @param destination       GUID of the destination
 * @param slot              Dependence "slot" on the EDT/Event to connect to
 *                          (up to n-1 where n is the number of dependences
 *                          for the EDT/Event)
 * @param mode              Access mode for the data-block (R/O, intent-to-write
 *                          or exclusive access)
 *
 * @return Status:
 *      - 0: Success
 *      - EINVAL: The slot number is invalid
 *      - ENOPERM: The source and destination GUIDs cannot be linked with
 *                 a dependence
 */
u8 ocrAddDependence(ocrGuid_t source,
                    ocrGuid_t destination, u32 slot,
                    ocrDbAccessMode_t mode /* = DB_DEFAULT_MODE */);

/**
   @}
**/
#ifdef __cplusplus
}
#endif

#endif /* __OCR_EDT_H__ */
