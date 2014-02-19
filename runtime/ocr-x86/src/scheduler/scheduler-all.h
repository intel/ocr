/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __SCHEDULER_ALL_H__
#define __SCHEDULER_ALL_H__

#include "ocr-config.h"
#include "utils/ocr-utils.h"

#include "ocr-scheduler.h"
#include "scheduler/hc/hc-scheduler.h"
#include "scheduler/ce/ce-scheduler.h"
#include "scheduler/xe/xe-scheduler.h"

typedef enum _schedulerType_t {
    schedulerHc_id,
    schedulerXe_id,
    schedulerCe_id,
    schedulerHcPlaced_id,
    schedulerNull_id,
    schedulerMax_id
} schedulerType_t;

extern const char * scheduler_types[];

ocrSchedulerFactory_t * newSchedulerFactory(schedulerType_t type, ocrParamList_t *perType);

#endif /* __SCHEDULER_ALL_H__ */
