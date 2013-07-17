/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __HC_SCHEDULER_H__
#define __HC_SCHEDULER_H__

#include "ocr-scheduler.h"
#include "ocr-types.h"
#include "ocr-utils.h"
#include "ocr-workpile.h"

typedef struct {
    ocrSchedulerFactory_t base;
} ocrSchedulerFactoryHc_t;

typedef struct {
    ocrScheduler_t scheduler;
    // Note: cache steal iterators in hc's scheduler
    // Each worker has its own steal iterator instantiated
    // a sheduler's construction time.
    ocrWorkpileIterator_t ** stealIterators;
    u64 workerIdFirst;
} ocrSchedulerHc_t;

typedef struct _paramListSchedulerHcInst_t {
    paramListSchedulerInst_t base;
    u64 workerIdFirst;
} paramListSchedulerHcInst_t;

ocrSchedulerFactory_t * newOcrSchedulerFactoryHc(ocrParamList_t *perType);

#endif /* __HC_SCHEDULER_H__ */
