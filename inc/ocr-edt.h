/**
 * @brief Event Driven Tasks, events and dependences
 * API
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-09-21
 * Copyright (c) 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/


#ifndef __OCR_EDT_H__
#define __OCR_EDT_H__

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
 * @warning Currently, only OCR_EVENT_STICKY_T work properly with the HC runtime.
 */
typedef enum {
    OCR_EVENT_ONCE_T,    /**< The event is automatically destroyed on satisfaction */
    OCR_EVENT_IDEM_T,    /**< The event exists until explicitly destroyed with
                          * ocrEventDestroy. It is satisfied once and susequent
                          * satisfactions are ignored (use case: BFS, B&B..) */
    OCR_EVENT_STICKY_T,  /**< The event exists until explicitly destroyed with
                          * ocrEventDestroy. Multiple satisfactions result
                          * in an error */
    OCR_EVENT_T_MAX      /**< Marker */
} ocrEventTypes_t;

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
 * @param eventGuid       GUID of event to satisfy
 * @param dataGuid        GUID of data to pass along or INVALID_GUID if no data
 * @return 0 on success and an error code on failure:
 *     - ENOMEM: Returned if there is not enough memory. This is usually caused by
 *               a programmer error (two events to a unique slot and both events are
 *               satisfied)
 *     - EINVAL: If the GUIDs do not refer to valid events/data-blocks
 *     - ENOPERM: If the event has already been satisfied or if the event does not take
 *                an argument and one is given or if the event takes an argument and none
 *                is given
 *
 * @note On satisfaction, a OCR_EVENT_ONCE_T event will pass the GUID of the
 * optional attached data-block to all EDTs/Events waiting on it at that time
 * and the event will destroy itself. For OCR_EVENT_IDEM_T and OCR_EVENT_STICKY_T,
 * the event will be satisfied for all EDTs/Events currently waiting on it
 * as well as any future EDT/Event that adds it as a dependence.
 **/
u8 ocrEventSatisfy(ocrGuid_t eventGuid, ocrGuid_t dataGuid /*= INVALID_GUID*/);

/**
   @}
**/

/**
 * @defgroup OCR EDT Event Driven Task Management
 * @brief APIs to manage the EDT in OCR
 *
 * @todo Re-evaluate the notion of event types (post 0.7 version)
 * @{
 **/

/**
 * @brief Type of the input passed to each EDT
 *
 * This struct associates a DB GUID with its base pointer (implicitly acquired
 * through ocrDbAcquire)
 */
typedef struct {
    ocrGuid_t guid;
    void* ptr;
} ocrEdtDep_t;


//
// EDTs properties bits
//

#define EDT_PROP_NONE   ((u16) 0)
#define EDT_PROP_FINISH ((u16) 1)

/**
 * @brief Type for an EDT
 *
 * @param paramc          Number of non-DB or non-event parameters. Use for simple types
 * @param params          Sizes for the parameters (to allow marshalling)
 * @param paramv          Values for non-DB and non-event parameters
 * @param depc            Number of dependences (either DBs or events)
 * @param depv            Values of the dependences. Can be INVAL_GUID/NULL if a pure control-flow event
 *                        was used as a dependence
 * @return Error code (0 on success)
 **/
typedef ocrGuid_t (*ocrEdt_t )( u32 paramc, u64 * params, void* paramv[],
                   u32 depc, ocrEdtDep_t depv[]);

/**
 * @brief Creates an EDT instance
 *
 * @param guid              Returned value: GUID of the newly created EDT type
 * @param funcPtr           Function to execute as the EDT
 * @param paramc            Number of non-DB and non-event parameters
 * @param params            Sizes for these parameters (to allow marshalling)
 * @param paramv            Values for those parameters (copied in)
 * @param properties        Reserved for future use
 * @param depc              Number of dependences for this EDT
 * @param depv              Values for the GUIDs of the dependences (if known)
 *                          If NULL, use ocrAddDependence
 * @param outputEvent       Event satisfied by the runtime when the edt has been executed.
 * @return 0 on success and an error code on failure: TODO
 **/
u8 ocrEdtCreate(ocrGuid_t * guid, ocrEdt_t funcPtr,
                u32 paramc, u64 * params, void** paramv,
                u16 properties, u32 depc, ocrGuid_t * depv, ocrGuid_t outputEvent);

/**
 * @brief Makes the EDT available for scheduling to the runtime
 *
 * This call should be called after ocrEdtCreate to signal to the runtime
 * that the EDT is ready to be scheduled. Note that this does *not* imply
 * that its dependences are satisfied. Instead, the runtime will start
 * considering the EDT for potential scheduling once its dependences are
 * satisfied. This call brings it to the attention of the runtime
 *
 * @param guid              GUID of the EDT to schedule
 * @return Status:
 *      - 0: Successful
 *      - EINVAL: The GUID is not an EDT
 *      - ENOPERM: The EDT does not have all its dependences known
 *
 * @warning In the current implementation, this call should only be called
 * after all dependences have been added using ocrAddDependence. This may
 * be relaxed in future versions
 */
u8 ocrEdtSchedule(ocrGuid_t guid);

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
 * @brief Adds a dependence from an event or a DB to an EDT
 *
 * The source GUID can be either an event or a DB. Giving a DB
 * as the source of a dependence is semantically equivalent to
 * giving a sticky event pre-satisfied with the data-block
 *
 * @param source            GUID of the event/DB for the EDT to depend on
 * @param destination       GUID for the EDT
 * @param slot              Dependence "slot" on the EDT to connect to (up to n-1
 *                          where n is the number of dependences for the EDT)
 *
 * @return Status:
 *      - 0: Success
 *      - EINVAL: The slot number is invalid
 *      - ENOPERM: The source and destination GUIDs cannot be linked with
 *                 a dependence
 * @todo Re-evaluate the chaining of events to allow for fan-out (post 0.7)
 */
u8 ocrAddDependence(ocrGuid_t source,
                    ocrGuid_t destination, u32 slot);

/**
   @}
**/
#endif /* __OCR_EDT_H__ */
