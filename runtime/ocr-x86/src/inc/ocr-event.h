/**
 * @brief OCR interface to events
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_EVENT_H__
#define __OCR_EVENT_H__

#include "ocr-edt.h"
#include "ocr-runtime-types.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

/**
 * @brief Parameter list to create an event factory
 */
typedef struct _paramListEventFact_t {
    ocrParamList_t base;
} paramListEventFact_t;


/****************************************************/
/* OCR EVENT                                        */
/****************************************************/

struct _ocrEvent_t;

/**
 * @brief Abstract class to represent OCR events function pointers
 *
 * This class provides the interface for the underlying implementation to conform with.
 */
typedef struct _ocrEventFcts_t {
    /**
     * @brief Virtual destructor for the Event interface
     * @param[in] self          Pointer to this event
     */
    u8 (*destruct)(struct _ocrEvent_t* self);

    /**
     * @brief Interface to get the GUID of the entity that satisfied an event.
     * @param[in] self          Pointer to this event
     * @return GUID of the entity the event has been satisfied with
     */
    ocrFatGuid_t (*get)(struct _ocrEvent_t* self);

    /**
     * @brief Interface to satisfy the event
     * @param[in] self         Pointer to this event
     * @param[in] db           GUID to satisfy this event with (or NULL_GUID)
     * @param[in] slot         Input slot for this event
     *
     * @return 0 on success and a non-zero code on failure
     */
    u8 (*satisfy)(struct _ocrEvent_t* self, ocrFatGuid_t db, u32 slot);

    /**
     * @brief Register a "signaler" on the event
     *
     * A signaler can either be another event or a data-block. If a data-block,
     * this call is equivalent to calling satisfy.
     *
     * @param[in] self          Pointer to this event
     * @param[in] signaler      GUID of the source (signaler)
     * @param[in] slot          Slot on self that will be satisfied by the signaler
     * @param[in] isDepAdd      True if this call is part of adding a dependence. False
     *                          if due to a standalone call
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*registerSignaler)(struct _ocrEvent_t *self, ocrFatGuid_t signaler, u32 slot,
                           bool isDepAdd);

    /**
     * @brief Oppositor of registerSignaler()
     *
     * This call will unregister the signaler from the event. Note that
     * the unregistration is per slot (ie: if a signaler is attached to
     * multiple slots, it will need to be unregistered from all slots
     *
     * @param[in] self          Pointer to this event
     * @param[in] signaler      GUID of the source (signaler)
     * @param[in] slot          Slot on self that will be satisfied by the signaler
     * @param[in] isDepRem      True if this call is part of removing a dependence
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*unregisterSignaler)(struct _ocrEvent_t *self, ocrFatGuid_t signaler, u32 slot,
                             bool isDepRem);

    /**
     * @brief Register a "waiter" (aka a dependence) on the event
     *
     * The waiter will be notified on slot 'slot' once this event is satisfid.
     * In other words, the satisfy() function serves to notify the "front" of
     * the event and this call serves to determine what happens at the "back"
     * of the event once the event is satisfied
     *
     * @param[in] self          Pointer to this event
     * @param[in] waiter        EDT/Event to register as a waiter
     * @param[in] slot          Slot to satisfy waiter on once this event
     *                          is satisfied
     * @param[in] isDepAdd      True if this call is part of adding a dependence.
     *                          False if due to a standalone call. This will be
     *                          important for STICKY events for example where
     *                          we only register waiters on their dependence frontier
     * @return 0 on success and a non-zero code on failure
     */
    u8 (*registerWaiter)(struct _ocrEvent_t *self, ocrFatGuid_t waiter, u32 slot,
                         bool isDepAdd);

    /**
     * @brief Unregisters a "waiter" (aka a dependence) on the event
     *
     * Note again that if a waiter is registered multiple times (for multiple
     * slots), you will need to unregister it multiple time as well.
     *
     * @param[in] self          Pointer to this event
     * @param[in] waiter        EDT/Event to register as a waiter
     * @param[in] slot          Slot to satisfy waiter on once this event
     *                          is satisfied
     * @param[in] isDepRem      True if part of removing a dependence
     * @return 0 on success and a non-zero code on failure
     */
    u8 (*unregisterWaiter)(struct _ocrEvent_t *self, ocrFatGuid_t waiter, u32 slot,
                           bool isDepRem);
} ocrEventFcts_t;

/**
 * @brief Abstract class to represent OCR events.
 *
 * Events can be satisfied with a GUID, polled for
 * what GUID satisfied the event and registered to
 * other events or edts.
 */
typedef struct _ocrEvent_t {
    ocrGuid_t guid;         /**< GUID for this event */
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcess_t *statProcess;
#endif
    ocrEventTypes_t kind;  /**< The kind of this event instance */
    u32 fctId;             /**< The functions to use to access this event */
} ocrEvent_t;


/****************************************************/
/* OCR EVENT FACTORY                                */
/****************************************************/

/**
 * @brief events factory
 */
typedef struct _ocrEventFactory_t {
    /** @brief Instantiates an Event and returns its corresponding GUID
     *  @param[in] factory          Pointer to this factory
     *  @param[in] eventType        Type of event to instantiate
     *  @param[in] takesArg         Does the event will take an argument
     *  @param[in] instanceArg      Arguments specific for this instance
     *  @return a new instance of an event
     */
    ocrEvent_t* (*instantiate)(struct _ocrEventFactory_t* factory, ocrEventTypes_t eventType,
                               bool takesArg, ocrParamList_t *instanceArg);

    /** @brief Virtual destructor for the factory
     *  @param[in] factory          Pointer to this factory
     */
    void (*destruct)(struct _ocrEventFactory_t* factory);

    u32 factoryId;             /**< Factory ID (matches fctId in event */
    ocrEventFcts_t fcts[OCR_EVENT_T_MAX]; /**< Functions for all the types of events */
} ocrEventFactory_t;

#endif /* __OCR_EVENT_H_ */
