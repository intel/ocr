/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __HC_EVENT_H__
#define __HC_EVENT_H__

#include "hc/hc.h"
#include "ocr-event.h"
#include "ocr-types.h"
#include "ocr-utils.h"
#include "ocr-sync.h"

typedef struct {
    ocrEventFactory_t base_factory;
    ocrEventFcts_t finishLatchFcts;
} ocrEventFactoryHc_t;

typedef struct ocrEventHc_t {
    ocrEvent_t base;
    ocrEventTypes_t kind;
} ocrEventHc_t;

typedef struct ocrEventHcAwaitable_t {
    ocrEventHc_t base;
    volatile regNode_t * waiters;
    volatile regNode_t * signalers;
    ocrGuid_t data;
} ocrEventHcAwaitable_t;

typedef struct ocrEventHcSingle_t {
    ocrEventHcAwaitable_t base;
} ocrEventHcSingle_t;

typedef struct ocrEventHcOnce_t {
    ocrEventHcAwaitable_t base;
    ocrAtomic64_t * nbEdtRegistered;
} ocrEventHcOnce_t;

typedef struct ocrEventHcLatch_t {
    ocrEventHcAwaitable_t base;
    //TODO use ocrAtomic but overhead may suck
    volatile int counter;
} ocrEventHcLatch_t;

typedef struct ocrEventHcFinishLatch_t {
    ocrEventHc_t base;
    // Dependences to be signaled
    regNode_t outputEventWaiter;
    regNode_t parentLatchWaiter; // Parent latch when nesting finish scope
    ocrGuid_t ownerGuid; // finish-edt starting the finish scope
    volatile ocrGuid_t returnGuid;
    //TODO use ocrAtomic but overhead may suck
    volatile int counter;
} ocrEventHcFinishLatch_t;

ocrEventFactory_t* newEventFactoryHc(ocrParamList_t *perType);

#endif /* __HC_EVENT_H__ */
