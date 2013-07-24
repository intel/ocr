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
void registerDependence(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot);


/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

typedef struct _paramListEventFact_t {
    ocrParamList_t base;
} paramListEventFact_t;


/****************************************************/
/* OCR EVENT                                        */
/****************************************************/

struct _ocrEvent_t;
/*! \brief Abstract class to represent OCR events function pointers
 *
 *  This class provides the interface for the underlying implementation to conform.
 */
typedef struct _ocrEventFcts_t {

    /*! \brief Virtual destructor for the Event interface
     */
    void (*destruct)(struct _ocrEvent_t* self);

    /*! \brief Interface to get the GUID of the entity that satisfied an event.
     *  \return GUID of the entity that satisfied this event
     */
    ocrGuid_t (*get) (struct _ocrEvent_t* self, u32 slot);

    /*! \brief Interface to satisfy the event
     *  \param[in]  db   GUID to satisfy this event with (or NULL_GUID)
     *  \param[in]  slot Input slot for this event (for latch events for example)
     */
    void (*satisfy)(struct _ocrEvent_t* self, ocrGuid_t db, u32 slot);
} ocrEventFcts_t;

/*! \brief Abstract class to represent OCR events.
 *
 *  This class provides the interface for the underlying implementation to conform.
 *  Events can be satisfied once with a GUID, can be polled for what GUID satisfied the event,
 *  and can be registered to by a task.
 */
typedef struct _ocrEvent_t {
    ocrGuid_t guid; /**< GUID for this event */
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcess_t statProcess;
#endif

    ocrEventTypes_t kind;
    /*! \brief Holds function pointer to the event interface
     */
    ocrEventFcts_t *fctPtrs;
} ocrEvent_t;


/****************************************************/
/* OCR EVENT FACTORY                                */
/****************************************************/

/*! \brief Abstract factory class to create OCR events.
 *
 *  This class provides an interface to create Event instances with a non-static create function
 *  to allow runtime implementers to choose to have state in their derived ocrEventFactory_t classes.
 */
typedef struct _ocrEventFactory_t {
    ocrMappable_t module;
    /*! \brief Instantiates an Event and returns its corresponding GUID
     *  \return Event metadata for the instantiated event
     */
    ocrEvent_t* (*instantiate)(struct _ocrEventFactory_t* factory, ocrEventTypes_t eventType,
                               bool takesArg, ocrParamList_t *perInstance);

    /*! \brief Virtual destructor for the ocrEventFactory_t interface
     */
    void (*destruct)(struct _ocrEventFactory_t* factory);

    ocrEventFcts_t singleFcts; /**< Functions for non-latch events */
    ocrEventFcts_t latchFcts;  /**< Functions for latch events */
} ocrEventFactory_t;

#endif /* __OCR_EVENT_H_ */
