/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __SCHEDULER_ALL_H__
#define __SCHEDULER_ALL_H__

#include "ocr-config.h"
#include "ocr-scheduler.h"
#include "utils/ocr-utils.h"

typedef enum _schedulerType_t {
    schedulerHc_id,
    schedulerXe_id,
    schedulerCe_id,
    schedulerHcPlaced_id,
    schedulerNull_id,
    schedulerMax_id
} schedulerType_t;

const char * scheduler_types[] = {
    "HC",
    "XE",
    "CE",
    "HC_Placed",
    "null",
    NULL
};

#include "scheduler/hc/hc-scheduler.h"
#include "scheduler/ce/ce-scheduler.h"
#include "scheduler/xe/xe-scheduler.h"

inline ocrSchedulerFactory_t * newSchedulerFactory(schedulerType_t type, ocrParamList_t *perType) {
    switch(type) {
#ifdef ENABLE_SCHEDULER_HC
    case schedulerHc_id:
        return newOcrSchedulerFactoryHc(perType);
#endif
#ifdef ENABLE_SCHEDULER_CE
    case schedulerCe_id:
        return newOcrSchedulerFactoryCe(perType);
#endif
#ifdef ENABLE_SCHEDULER_XE
    case schedulerXe_id:
        return newOcrSchedulerFactoryXe(perType);
#endif
    case schedulerHcPlaced_id:
    default:
        ASSERT(0);
    }
    return NULL;
}

#endif /* __SCHEDULER_ALL_H__ */
