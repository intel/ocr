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
    ocrEventFactory_t base;
} ocrEventFactoryHc_t;

typedef struct ocrEventHc_t {
    ocrEvent_t base;
    ocrFatGuid_t waitersDb; /**< DB containing an array of regNode_t listing the
                             * events/EDTs depending on this event */
    ocrFatGuid_t signalersDb; /**< DB containing an array of regNode_t listing
                               * the events/EDTs that will signal this event */
    u32 waitersCount; /**< Number of waiters in waitersDb */
    u32 waitersMax; /**< Maximum number of waiters in waitersDb */
    u32 signalersCount; /**< Number of signalers in signalersDb */
    u32 signalersMax; /**< Maximum number of signalers in signalersDb */
} ocrEventHc_t;

// STICKY or IDEM events need a lock
// NOTE: This is a sucky implementation for
// now but works
typedef struct _ocrEventHcPersist_t {
    ocrEventHc_t base;
    ocrGuid_t data;
    volatile u32 waitersLock;
} ocrEventHcPersist_t;
    

typedef struct ocrEventHcLatch_t {
    ocrEventHc_t base;
    volatile s32 counter;
} ocrEventHcLatch_t;


ocrEventFactory_t* newEventFactoryHc(ocrParamList_t *perType, u32 factoryId);

#endif /* ENABLE_EVENT_HC */
#endif /* __HC_EVENT_H__ */
