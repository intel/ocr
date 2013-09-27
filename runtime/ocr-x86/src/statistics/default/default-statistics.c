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
#include "ocr-macros.h"
#include "ocr-policy-domain-getter.h"
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

    ocrGuidKind guidK;
    guidKind(getCurrentPD(), processGuid, &guidK);
    switch(guidK) {
    case OCR_GUID_DB:
        intStatsProcessRegisterFilter(result, (0x3F<<((u32)STATS_DB_CREATE-1)), t, 0);
        break;
    case OCR_GUID_EDT:
        intStatsProcessRegisterFilter(result, 0x1F, t, 0);
        break;
        // Other cases: TODO
    default:
        ASSERT(0);
    };
    
    DPRINTF(DEBUG_LVL_INFO, "Stats 0x%lx: Created a StatsProcess (0x%lx) for object with GUID 0x%lx\n",
            (u64)self, (u64)result, processGuid);

    return result;
}

static void defaultDestructStatsProcess(ocrStats_t *self, ocrStatsProcess_t *process) {
    DPRINTF(DEBUG_LVL_INFO, "Destroying StatsProcess 0x%lx (GUID: 0x%lx)\n", (u64)process, process->me);
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

