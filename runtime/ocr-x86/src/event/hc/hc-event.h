/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __HC_EVENT_H__
#define __HC_EVENT_H__

#include "ocr-config.h"
#ifdef ENABLE_EVENT_HC

#include "hc/hc.h"
#include "ocr-event.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

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
    // TODO: This may not be required
    u64 nbEdtRegistered;
} ocrEventHcOnce_t;

typedef struct ocrEventHcLatch_t {
    ocrEventHcAwaitable_t base;
    volatile s32 counter;
} ocrEventHcLatch_t;

typedef struct ocrEventHcFinishLatch_t {
    ocrEventHc_t base;
    // Dependences to be signaled
    regNode_t outputEventWaiter;
    regNode_t parentLatchWaiter; // Parent latch when nesting finish scope
    ocrGuid_t ownerGuid; // finish-edt starting the finish scope
    volatile ocrGuid_t returnGuid;
    volatile s32 counter;
} ocrEventHcFinishLatch_t;

ocrEventFactory_t* newEventFactoryHc(ocrParamList_t *perType, u32 factoryId);

#endif /* __HC_EVENT_H__ */
#endif /* ENABLE_EVENT_HC */
