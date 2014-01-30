/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __CE_SCHEDULER_H__
#define __CE_SCHEDULER_H__

#include "ocr-config.h"
#ifdef ENABLE_SCHEDULER_CE

#include "ocr-scheduler.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

/******************************************************/
/* Support structures (workpile iterator)             */
/******************************************************/

struct _ocrWorkpile_t;

typedef struct _ceWorkpileIterator_t {
    struct _ocrWorkpile_t **workpiles;
    u64 id, curr, mod;
} ceWorkpileIterator_t;

typedef struct {
    ocrSchedulerFactory_t base;
} ocrSchedulerFactoryCe_t;

typedef struct {
    ocrScheduler_t scheduler;
    ceWorkpileIterator_t * stealIterators;
    u64 workerIdFirst;
} ocrSchedulerCe_t;

typedef struct _paramListSchedulerCeInst_t {
    paramListSchedulerInst_t base;
    u64 workerIdFirst;
} paramListSchedulerCeInst_t;

ocrSchedulerFactory_t * newOcrSchedulerFactoryCe(ocrParamList_t *perType);

#endif /* ENABLE_SCHEDULER_CE */
#endif /* __CE_SCHEDULER_H__ */

