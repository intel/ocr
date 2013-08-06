/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __FSIM_SCHEDULER_H__
#define __FSIM_SCHEDULER_H__

#include "ocr-scheduler.h"
#include "ocr-types.h"
#include "ocr-utils.h"
#include "ocr-workpile.h"

typedef struct {
    ocrSchedulerFactory_t base;
} ocrSchedulerFactoryCE_t;

typedef struct {
    ocrSchedulerFactory_t base;
} ocrSchedulerFactoryXE_t;

typedef struct {
    ocrScheduler_t scheduler;
    // Note: cache steal iterators in scheduler
    // Each worker has its own steal iterator instantiated
    // a scheduler's construction time.
    ocrWorkpileIterator_t ** stealIterators;
    u64 workerIdFirst;
    u64 workerIdLast;
    u64 nWorkers;
} ocrSchedulerXE_t;

typedef struct {
    ocrScheduler_t scheduler;
    // Note: cache steal iterators in scheduler
    // Each worker has its own steal iterator instantiated
    // a scheduler's construction time.
    ocrWorkpileIterator_t ** stealIterators;
    u64 workerIdFirst;
    u64 workerIdLast;
    u64 nWorkers;
} ocrSchedulerCE_t;

typedef struct _paramListSchedulerCEInst_t {
    paramListSchedulerInst_t base;
    u64 workerIdFirst;
    u64 workerIdLast;
    u64 nWorkers;
} paramListSchedulerCEInst_t;

typedef struct _paramListSchedulerXEInst_t {
    paramListSchedulerInst_t base;
    u64 workerIdFirst;
    u64 workerIdLast;
    u64 nWorkers;
} paramListSchedulerXEInst_t;

ocrSchedulerFactory_t * newOcrSchedulerFactoryCE (ocrParamList_t *perType);

ocrSchedulerFactory_t * newOcrSchedulerFactoryXE (ocrParamList_t *perType);

#endif /* __HC_SCHEDULER_H__ */

