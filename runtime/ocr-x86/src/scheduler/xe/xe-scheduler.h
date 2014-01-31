/*
 * This file is subject to the license agreement located in the file LIXENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __XE_SCHEDULER_H__
#define __XE_SCHEDULER_H__

#include "ocr-config.h"
#ifdef ENABLE_SCHEDULER_XE

#include "ocr-scheduler.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

typedef struct {
    ocrSchedulerFactory_t base;
} ocrSchedulerFactoryXe_t;

typedef struct {
    ocrScheduler_t scheduler;
} ocrSchedulerXe_t;

typedef struct _paramListSchedulerXeInst_t {
    paramListSchedulerInst_t base;
} paramListSchedulerXeInst_t;

ocrSchedulerFactory_t * newOcrSchedulerFactoryXe(ocrParamList_t *perType);

#endif /* ENABLE_SCHEDULER_XE */
#endif /* __XE_SCHEDULER_H__ */

