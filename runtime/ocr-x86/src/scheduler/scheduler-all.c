/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "scheduler/scheduler-all.h"
#include "debug.h"

const char * scheduler_types[] = {
    "HC",
    "XE",
    "CE",
    "HC_Placed",
    "null",
    NULL
};

ocrSchedulerFactory_t * newSchedulerFactory(schedulerType_t type, ocrParamList_t *perType) {
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

void initializeSchedulerOcr(ocrSchedulerFactory_t * factory, ocrScheduler_t * self, ocrParamList_t *perInstance) {
    self->fguid.guid = UNINITIALIZED_GUID;
    self->fguid.metaDataPtr = self;
    self->pd = NULL;
    self->workpiles = NULL;
    self->workpileCount = 0;
    self->fcts = factory->schedulerFcts;
}
