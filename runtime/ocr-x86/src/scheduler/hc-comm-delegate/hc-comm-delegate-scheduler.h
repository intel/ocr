/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __HC_COMM_DELEGATE_SCHEDULER_H__
#define __HC_COMM_DELEGATE_SCHEDULER_H__

#include "ocr-config.h"
#ifdef ENABLE_SCHEDULER_HC_COMM_DELEGATE

#include "ocr-types.h"
#include "utils/ocr-utils.h"
#include "utils/deque.h"
#include "scheduler/hc/hc-scheduler.h"

typedef struct {
    ocrSchedulerFactoryHc_t base;
    // cached base function pointers
    void (*baseInitialize)(struct _ocrSchedulerFactory_t * factory,
                                      struct _ocrScheduler_t *self, ocrParamList_t *perInstance);
    void (*baseStart)(struct _ocrScheduler_t *self, struct _ocrPolicyDomain_t * PD);
    void (*baseStop)(struct _ocrScheduler_t *self);
    u8 (*baseMonitorProgress)(struct _ocrScheduler_t *self, ocrMonitorProgress_t type, void * monitoree);
} ocrSchedulerFactoryHcComm_t;

typedef struct {
    ocrSchedulerHc_t base;
    u64 outboxesCount;
    deque_t ** outboxes;
    u64 inboxesCount;
    deque_t ** inboxes;
    // cached base function pointers
    void (*baseStart)(struct _ocrScheduler_t *self, struct _ocrPolicyDomain_t * PD);
    void (*baseStop)(struct _ocrScheduler_t *self);
    u8 (*baseMonitorProgress)(struct _ocrScheduler_t *self, ocrMonitorProgress_t type, void * monitoree);
} ocrSchedulerHcCommDelegate_t;

ocrSchedulerFactory_t * newOcrSchedulerFactoryHcCommDelegate(ocrParamList_t *perType);

#endif /* ENABLE_SCHEDULER_HC_COMM_DELEGATE */
#endif /* __HC_COMM_DELEGATE_SCHEDULER_H__ */

