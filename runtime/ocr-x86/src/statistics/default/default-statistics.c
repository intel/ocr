/**
 * @brief Implementation of a default statistics collector
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"
#include "ocr-utils.h"
#include "statistics/filters/filters-all.h"
#include "statistics/internal.h"
#include "statistics/messages/messages-all.h"

#include "default-statistics.h"

#define DEBUG_TYPE STATS

/******************************************************/
/* OCR STATISTICS DEFAULT IMPLEMENTATION              */
/******************************************************/

static void defaultDestruct(ocrStats_t *self) {
    ocrStatsDefault_t *rself = (ocrStatsDefault_t*)self;
    if(rself->aggregatingFilter)
        rself->aggregatingFilter->fcts.destruct(rself->aggregatingFilter);
    free(self);
}

static void defaultSetPD(ocrPolicyDomain_t *pd) {
    // Do nothing for now
}

static ocrStatsMessage_t* defaultCreateMessage(ocrStats_t *self,
        ocrStatsEvt_t type,
        ocrGuid_t src, ocrGuid_t dest,
        ocrStatsParam_t *instanceArg) {

    // In this case, we always create a trivial message
    // In the general case, you can switch on 'type' and pick an ocrStatsEvtInt_t
    // that is appropriate
    return newStatsMessage(STATS_MESSAGE_TRIVIAL, type, src, dest, instanceArg);
}

static void defaultDestructMessage(ocrStats_t *self, ocrStatsMessage_t* message) {

    message->fcts.destruct(message);
}

static ocrStatsProcess_t* defaultCreateStatsProcess(ocrStats_t *self, ocrGuid_t processGuid) {

    ocrStatsDefault_t *rself = (ocrStatsDefault_t*)self;
    ocrStatsProcess_t *result = intCreateStatsProcess(processGuid);

    // Goes here because cannot go in the initialization as PD is not
    // brought up at that point
    if(!rself->aggregatingFilter) {
        rself->aggregatingFilter = newStatsFilter(STATS_FILTER_FILE_DUMP,
                                   /*parent */NULL, /*param*/NULL);
    }

    ocrStatsFilter_t *t = newStatsFilter(STATS_FILTER_TRIVIAL, rself->aggregatingFilter, NULL);
    ocrStatsFilter_t *t2 = newStatsFilter(STATS_FILTER_TRIVIAL, rself->aggregatingFilter, NULL); // Create different
    // filters since each filter should only be attached to one thing
    // Will print out everything that goes in or out of the object
    intStatsProcessRegisterFilter(result, STATS_ALL, t, 0);
    intStatsProcessRegisterFilter(result, STATS_ALL, t2, 1);

    DPRINTF(DEBUG_LVL_INFO, "Stats 0x%lx: Created a StatsProcess 0x%lx for parent object GUID 0x%lx\n",
            (u64)self, (u64)result, processGuid);

    DPRINTF(DEBUG_LVL_WARN, "Stats 0x%lx: StatsProcess 0x%lx associated with IN filter 0x%lx and OUT filter 0x%lx\n",
            (u64)self, (u64)result, (u64)t, (u64)t2);
    return result;
}

static void defaultDestructStatsProcess(ocrStats_t *self, ocrStatsProcess_t *process) {
    DPRINTF(DEBUG_LVL_INFO, "Stats 0x%lx: Destroying StatsProcess 0x%lx (parent object GUID: 0x%lx)\n",
            (u64)self, (u64)process, process->me);
    intDestructStatsProcess(process);
}

static ocrStatsFilter_t* defaultGetFilter(ocrStats_t *self, ocrStats_t *requester,
        ocrStatsFilterType_t type) {

    // Unsupported at this point. This is to support linking filters across policy
    // domains. For now we have one policy domain
    ASSERT(0);
    return NULL;
}

static void defaultDumpFilter(ocrStats_t *self, ocrStatsFilter_t *filter, u64 tick, ocrStatsEvt_t type,
                              ocrGuid_t src, ocrGuid_t dest) {

    ASSERT(0); // Don't know what this does
}

// Method to create the default statistics colloector
static ocrStats_t * newStatsDefault(ocrStatsFactory_t * factory,
                                    ocrParamList_t *perInstance) {

    ocrStatsDefault_t *result = (ocrStatsDefault_t*)
                                checkedMalloc(result, sizeof(ocrStatsDefault_t));

    result->aggregatingFilter = NULL;

    result->base.fctPtrs = &(factory->statsFcts);

    return (ocrStats_t*)result;
}

/******************************************************/
/* OCR STATISTICS DEFAULT FACTORY                     */
/******************************************************/

static void destructStatsFactoryDefault(ocrStatsFactory_t * factory) {
    free(factory);
}

ocrStatsFactory_t * newStatsFactoryDefault(ocrParamList_t *perType) {
    ocrStatsFactory_t* base = (ocrStatsFactory_t*)
                              checkedMalloc(base, sizeof(ocrStatsFactoryDefault_t));
    base->instantiate = newStatsDefault;
    base->destruct =  &destructStatsFactoryDefault;
    base->statsFcts.destruct = &defaultDestruct;
    base->statsFcts.setContainingPD = &defaultSetPD;
    base->statsFcts.createMessage = &defaultCreateMessage;
    base->statsFcts.destructMessage = &defaultDestructMessage;
    base->statsFcts.createStatsProcess = &defaultCreateStatsProcess;
    base->statsFcts.destructStatsProcess = &defaultDestructStatsProcess;
    base->statsFcts.getFilter = &defaultGetFilter;
    base->statsFcts.dumpFilter = &defaultDumpFilter;
    return base;
}

#endif /* OCR_ENABLE_STATISTICS */
