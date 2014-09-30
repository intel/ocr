/**
 * @brief OCR APIs for EDTs, events and dependences
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
 * @brief Creates an event
 *
 * This function creates a new programmer-managed event identified by the
 * returned GUID.
 *
 * @param[out] guid       The GUID created by the runtime for the new event
 * @param[in] eventType   The type of event to create. See #ocrEventTypes_t
 * @param[in] takesArg    True if this event will potentially carry a data block on
 *                        satisfaction, false othersie
 * @return a status code
 *     - 0: successful
 *     - ENOMEM: If space cannot be found to allocate the event
 *     - EINVAL: If eventType was malformed or is incompatible with takesArg
 **/
u8 ocrEventCreate(ocrGuid_t *guid, ocrEventTypes_t eventType, bool takesArg);

/**
 * @brief Explicitly destroys an event
 *
 * Events such as ONCE or LATCH are automatically destroyed once
 * they trigger; others, however, need to be explicitly destroyed
 * by the programmer. This call enables this.
 *
 * @param[in] guid     The GUID of the event to destroy
 * @return a statuc code
 *     - 0: successful
 *     - EINVAL: If guid does not refer to a valid event to destroy
 **/
u8 ocrEventDestroy(ocrGuid_t guid);

/**
 * @brief Satisfy the first pre-slot of an event
 * and optionally pass a data block along to the event
 *
 * Satisfying the pre-slot of an event will potentially
 * trigger the satisfaction of its post-slot depending on its
 * trigger rule:
 *     - ONCE, IDEM and STICKY events will satisfy their
 *       post-slot upon satisfaction of their pre-slot
 *     - LATCH events have a more complex rule; see #ocrEventTypes_t.
 *
 * During satisfaction, the programmer may associate a
 * data block to the pre-slot of the event. Depending on the event
 * type, that data block will be passed along to its post-slot:
 *     - ONCE, IDEM and STICKY events will pass along the data block
 *       to their post-slot
 *     - LATCH events ignore any data block passed
 *
 * @param[in] eventGuid  GUID of the event to satisfy
 * @param[in] dataGuid   GUID of the data block to pass along or NULL_GUID
 *
 * @return a status code
 *     - 0: successful
 *     - ENOMEM: If there is not enough memory. This is usually caused by
 *               a programmer error
 *     - EINVAL: If the GUIDs do not refer to valid events/data blocks
 *     - ENOPERM: If the event has already been satisfied or if the event does not take
 *                an argument and one is given or if the event takes an argument and none
 *                is given
 *
 * An event satisfaction without the optional data block can be viewed as a pure
 * control dependence whereas one with a data block is a control+data dependence
 *
 * @note On satisfaction, a ONCE event will pass the GUID of the
 * optionaly attached data block to all OCR objects waiting on it at that time
 * and the event will destroy itself. IDEM and STICKY events will pass the GUID
 * of the optionaly attached data block to all OCR objects waiting on it at that
 * time as well as any new objects linked to it until the event is destroyed.
 **/
u8 ocrEventSatisfy(ocrGuid_t eventGuid, ocrGuid_t dataGuid /*=NULL_GUID*/);

/**
 * @brief Satisfy the specified pre-slot of an event
 *
 * This call is used primarily for LATCH events.
 * ocrEventSatisfySlot(eventGuid, dataGuid, 0) is equivalent to
 * ocrEventSatisfy(eventGuid, dataGuid)
 *
 * @see ocrEventSatisfy()
 *
 * @param[in] eventGuid  GUID of the event to satisfy
 * @param[in] dataGuid   GUID of the data block to pass along or NULL_GUID
 * @param[in] slot       Pre-slot on the destination event to satisfy
 *
 * @return a status code
 *     - 0: successful
 *     - ENOMEM: If there is not enough memory. This is usually caused by
 *               a programmer error
 *     - EINVAL: If the GUIDs do not refer to valid events/data blocks
 *     - ENOPERM: If the event has already been satisfied or if the event does not take
 *                an argument and one is given or if the event takes an argument and none
 *                is given
 */
u8 ocrEventSatisfySlot(ocrGuid_t eventGuid, ocrGuid_t dataGuid, u32 slot);

/**
   @}
**/

/**
 * @defgroup OCREDT Event Driven Task API
 * @brief APIs to manage EDTs in OCR
 *
 * @{
 **/
/**
 * @brief Creates an EDT template
 *
 * An EDT template encapsulates the EDT function and, optionally, the number
 * of parameters and arguments that EDTs instanciated from this template will use.
 * It needs to be created only once for each function that will serve as an EDT; reusing
 * the same template from multiple EDTs of the same type may improve performance as this
 * allows the runtime to collect information about multiple instances of the same
 * type of EDT.
 *
 * @param[out] guid       Runtime created GUID for the newly created EDT Template
 * @param[in] funcPtr     EDT function. This function must be of type #ocrEdt_t.
 * @param[in] paramc      Number of parameters that the EDTs will take. If not known
 *                        or variable, this can be EDT_PARAM_UNK
 * @param[in] depc        Number of pre-slots that the EDTs will have. If not known
 *                        or variable, this can be EDT_PARAM_UNK
 * @param[in] funcName    User-specified name for the template (used in debugging)
 *
 * @return a status code:
 *      - 0: successful
 *      - ENOMEM: No memory available to allocate the template
 *
 * @note You should use ocrEdtTemplateCreate() as opposed to this
 * internal function. If OCR_ENABLE_EDT_NAMING is enabled, the C name
 * of the function will be used as funcName.
 */
u8 ocrEdtTemplateCreate_internal(ocrGuid_t *guid, ocrEdt_t funcPtr,
                                 u32 paramc, u32 depc, const char* funcName);


#ifdef OCR_ENABLE_EDT_NAMING
/**
 * @brief Creates an EDT template
 * @see ocrEdtTemplateCreate_internal()
 */
#define ocrEdtTemplateCreate(guid, funcPtr, paramc, depc)               \
    ocrEdtTemplateCreate_internal((guid), (funcPtr), (paramc), (depc), #funcPtr)
#else
/**
 * @brief Creates an EDT template
 * @see ocrEdtTemplateCreate_internal()
 */
#define ocrEdtTemplateCreate(guid, funcPtr, paramc, depc)               \
    ocrEdtTemplateCreate_internal((guid), (funcPtr), (paramc), (depc), NULL)
#endif

/**
 * @brief Destroy an EDT template
 *
 * This function can be called if no further EDTs will be created based on this
 * template
 *
 * @param[in] guid       GUID of the EDT template to destroy
 * @return a status code
 *      - 0: successful
 */
u8 ocrEdtTemplateDestroy(ocrGuid_t guid);

/**
 * @brief Creates an EDT instance from an EDT template
 *
 * @param[out] guid             GUID of the newly created EDT type
 * @param[in] templateGuid      GUID of the template to use to create this EDT
 * @param[in] paramc            Number of parameters (64-bit values). Set to #EDT_PARAM_DEF if
 *                              you want to use the 'paramc' value specified at the time of
 *                              the EDT template's creation
 * @param[in] paramv            64-bit values for the parameters. This must be an array
 *                              of paramc 64-bit values. These values are copied in. If
 *                              paramc is 0, this must be NULL.
 * @param[in] depc              Number of dependences for this EDT. Set to #EDT_PARAM_DEF if
 *                              you want to use the 'depc' value specified at the time
 *                              of the EDT template's creation
 * @param[in] depv              Values for the GUIDs of the dependences (if known). Note
 *                              that all dependences added by this method will be in the
 *                              #DB_DEFAULT_MODE. Use ocrAddDependence() to add unknown
 *                              dependences or dependences with another mode.
 *                              This pointer should either be NULL or point to an array
 *                              of size 'depc'.
 * @param[in] properties        Used to indicate if this is a finish EDT
 *                              (see #EDT_PROP_FINISH). Use #EDT_PROP_NONE as
 *                              a default value.
 * @param[in] affinity          Affinity container for this EDT. Can be NULL_GUID. This
 *                              is currently an experimental feature
 * @param[in,out] outputEvent   If not NULL on input, on successful return
 *                              of this call, this will return the GUID of the
 *                              event associated with the post-slot of the EDT.
 *                              For a FINISH EDT, the post-slot of this event will
 *                              be satisfied when the EDT and all of the child
 *                              EDTs have completed execution. For a non FINISH EDT,
 *                              the post-slot of this event will be satisfied when the
 *                              EDT completes execution and will carry the
 *                              data block returned by the EDT. If NULL, no event
 *                              will be associated with the EDT's post-slot
 *
 * @return a status code
 *      - 0: successful
 *
 **/
u8 ocrEdtCreate(ocrGuid_t * guid, ocrGuid_t templateGuid,
                u32 paramc, u64* paramv, u32 depc, ocrGuid_t *depv,
                u16 properties, ocrGuid_t affinity, ocrGuid_t *outputEvent);

/**
 * @brief Destroy an EDT
 *
 * EDTs are normally destroyed after they execute. This call
 * is provided if an EDT is created and the programmer later
 * realizes that it will never become runable.
 *
 * @param[in] guid     GUID of the EDT to destroy
 *
 * @return a status code
 *      - 0: successful
 */
u8 ocrEdtDestroy(ocrGuid_t guid);

/**
   @}
**/


/**
   @defgroup OCRDependences OCR dependence APIs
   @brief APIs to manage OCR dependences

   @{
**/

/**
 * @brief Adds a dependence between OCR objects:
 *
 * The following dependences can be added:
 *    - Event to Event: The sink event's pre-slot will become
 *                      satisfied upon satisfaction of the
 *                      source event's post-slot. Any data block
 *                      associated with the source event's post-slot
 *                      will be associated with the sink event's pres-slot.
 *    - Event to EDT  : Upon satisfaction of the source event's post-slot,
 *                      the pre-slot of the EDT will be satisfied. When the
 *                      EDT runs, the data block associated with the post-slot
 *                      of the event will be passed in through the depv
 *                      array. This represents a control or a control+data
 *                      dependence.
 *    - DB to Event   : ocrAddDependence(db, evt, slot) is equivalent
 *                      to ocrSatisfySlot(evt, db, slot)
 *    - DB to EDT     : This represents a pure data dependence. Adding
 *                      a dependence between a data block and an EDT
 *                      immediately satisfies the pre-slot of the EDT. When
 *                      the EDT runs, the data block will be passed in
 *                      through the depv array.
 *
 * @param[in] source       GUID of the source
 * @param[in] destination  GUID of the destination
 * @param[in] slot         Index of the pre-slot on the destination OCR object
 * @param[in] mode         Access mode of the destination for the data block. See
 *                         #ocrDbAccessMode_t.
 *
 * @return a status code
 *      - 0: successful
 *      - EINVAL: The slot number is invalid
 *      - ENOPERM: The source and destination GUIDs cannot be linked with
 *                 a dependence
 */
u8 ocrAddDependence(ocrGuid_t source, ocrGuid_t destination, u32 slot,
                    ocrDbAccessMode_t mode);

/**
   @}
**/
#ifdef __cplusplus
}
#endif

#endif /* __OCR_EDT_H__ */
