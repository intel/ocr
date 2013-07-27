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
#include "ocr-mappable.h"
#include "ocr-types.h"
#include "ocr-utils.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif


// Define internal finish-latch event id after user-level events
#define OCR_EVENT_FINISH_LATCH_T OCR_EVENT_T_MAX+1


/*******************************************
 * Dependence Registration
 ******************************************/

   /**
    * @brief Registers dependence between two guids.
    *
    * @param signalerGuid          Guid 'signaling'
    * @param waiterGuid            The guid to be satisfied
    * @param slot                  The slot to signal the waiterGuid on.
    */
void registerDependence(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot);


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

/*! \brief Abstract class to represent OCR events function pointers
 *
 *  This class provides the interface for the underlying implementation to conform with.
 */
typedef struct _ocrEventFcts_t {
    /*! \brief Virtual destructor for the Event interface
     *  \param[in] self          Pointer to this event
     */
    void (*destruct)(struct _ocrEvent_t* self);

    /*! \brief Interface to get the GUID of the entity that satisfied an event.
     *  \param[in] self          Pointer to this event
     *  \param[in] slot          The slot of the event to get from
     *  \return GUID of the entity the event has been satisfied with
     */
    ocrGuid_t (*get) (struct _ocrEvent_t* self, u32 slot);

    /*! \brief Interface to satisfy the event
     *  \param[in] self          Pointer to this event
     *  \param[in] db            GUID to satisfy this event with (or NULL_GUID)
     *  \param[in] slot          Input slot for this event
     */
    void (*satisfy)(struct _ocrEvent_t* self, ocrGuid_t db, u32 slot);
} ocrEventFcts_t;

/*! \brief Abstract class to represent OCR events.
 *
 *  Events can be satisfied with a GUID, polled for 
 *  what GUID satisfied the event and registered to
 *  other events or edts.
 */
typedef struct _ocrEvent_t {
    ocrGuid_t guid; /**< GUID for this event */
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcess_t statProcess;
#endif
    ocrEventTypes_t kind;  /**< The kind of this event instance */
    ocrEventFcts_t *fctPtrs;  /**< Function pointers for this instance */
} ocrEvent_t;


/****************************************************/
/* OCR EVENT FACTORY                                */
/****************************************************/

/**
 * @brief events factory
 */
typedef struct _ocrEventFactory_t {
    /*! \brief Instantiates an Event and returns its corresponding GUID
     *  \param[in] factory          Pointer to this factory
     *  \param[in] eventType        Type of event to instantiate
     *  \param[in] takesArg         Does the event will take an argument
     *  \param[in] instanceArg      Arguments specific for this instance
     *  \return a new instance of an event
     */
    ocrEvent_t* (*instantiate)(struct _ocrEventFactory_t* factory, ocrEventTypes_t eventType,
                               bool takesArg, ocrParamList_t *instanceArg);

    /*! \brief Virtual destructor for the factory
     *  \param[in] factory          Pointer to this factory
     */
    void (*destruct)(struct _ocrEventFactory_t* factory);

    ocrEventFcts_t singleFcts; /**< Functions for non-latch events */
    ocrEventFcts_t latchFcts;  /**< Functions for latch events */
} ocrEventFactory_t;

#endif /* __OCR_EVENT_H_ */
