/**
 * @brief OCR events
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __EVENT_ALL_H__
#define __EVENT_ALL_H__

#include "debug.h"
#include "ocr-event.h"
#include "ocr-utils.h"

typedef enum _eventType_t {
    eventHc_id,
    eventFsim_id,
    eventMax_id
} eventType_t;

const char * event_types [] = {
    "HC",
    "FSIM",
    NULL
};

// HC Event
#include "event/hc/hc-event.h"

// Add other events using the same pattern as above

inline ocrEventFactory_t *newEventFactory(eventType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case eventHc_id:
        return newEventFactoryHc(typeArg);
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __EVENT_ALL_H__ */
