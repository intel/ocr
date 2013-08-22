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
#include "ocr-statistics.h"
#include "ocr-types.h"
#include "statistics/internal.h"

#include <stdlib.h>
#include <string.h>

#define DEBUG_TYPE STATS

ocrStatsProcess_t* intCreateStatsProcess(ocrGuid_t processGuid) {
    ocrStatsProcess_t *result = checkedMalloc(result, sizeof(ocrStatsProcess_t));
    
    u64 i;
    ocrPolicyDomain_t *policy = getCurrentPD();
    ocrPolicyCtx_t *ctx = getCurrentWorkerContext();
    
    result->me = processGuid;
    result->processing = policy->getLock(policy, ctx);
    result->messages = policy->getQueue(policy, 32, ctx);
    result->tick = 0;
    
    // +1 because the first bucket keeps track of all filters uniquely
    result->filters = (ocrStatsFilter_t***)checkedMalloc(result->filters,
                                                        sizeof(ocrStatsFilter_t**)*(STATS_EVT_MAX+1));
    result->filterCounts = (u64*)checkedMalloc(result->filterCounts, sizeof(u64)*(STATS_EVT_MAX+1));
    
    for(i = 0; i < (u64)STATS_EVT_MAX + 1; ++i) result->filterCounts[i] = 0ULL;
    return result;
}

void intDestructStatsProcess(ocrStatsProcess_t *process) {
    u64 i;
    u32 j;


    // Make sure we process all remaining messages
    while(!process->processing->fctPtrs->trylock(process->processing)) ;

    while(intProcessMessage(process)) ;

    // Free all the filters associated with this process
    // Assumes that the filter is only associated with ONE
    // process
    u32 maxJ = (u32)(process->filterCounts[0] & 0xFFFFFFFF);
    for(j = 0; j < maxJ; ++j) {
        process->filters[0][j]->fcts.destruct(process->filters[0][j]);
    }
    if(maxJ)
        free(process->filters[0]);
    for(i = 1; i < (u64)STATS_EVT_MAX + 1; ++i) {
        if(process->filterCounts[i])
            free(process->filters[i]);
    }

    free(process->filters);
    free(process->filterCounts);

    process->processing->fctPtrs->unlock(process->processing);
    process->processing->fctPtrs->destruct(process->processing);

    process->messages->fctPtrs->destruct(process->messages);

}

void intStatsProcessRegisterFilter(ocrStatsProcess_t *self, u64 bitMask,
                                   ocrStatsFilter_t *filter) {

    DPRINTF(DEBUG_LVL_VERB, "Registering filter 0x%lx with process 0x%lx for mask 0x%lx\n",
            (u64)filter, (u64)self, bitMask);
    u32 countAlloc, countPresent;

    while(bitMask) {
        // Setting some filter up
        u64 bitSet = fls64(bitMask);
        bitMask &= ~(1ULL << bitSet);

        ASSERT(bitSet < STATS_EVT_MAX);
        ++bitSet; // 0 is all filters uniquely

        // Check to make sure we have enough room to add this filter
        countPresent = (u32)(self->filterCounts[bitSet] & 0xFFFFFFFF);
        countAlloc = (u32)(self->filterCounts[bitSet] >> 32);
        //     DPRINTF(DEBUG_LVL_VVERB, "For type %ld, have %d present and %d allocated (from 0x%lx)\n",
        //            bitSet, countPresent, countAlloc, self->filterCounts[bitSet]);
        if(countAlloc <= countPresent) {
            // Allocate using an exponential model
            if(countAlloc)
                countAlloc *= 2;
            else
                countAlloc = 1;
            ocrStatsFilter_t **newAlloc = (ocrStatsFilter_t**)malloc(sizeof(ocrStatsFilter_t*)*countAlloc);
            // TODO: This memcpy needs to be replaced. The malloc above as well...
            memcpy(newAlloc, self->filters[bitSet], countPresent*sizeof(ocrStatsFilter_t*));
            if(countAlloc != 1)
                free(self->filters[bitSet]);
            self->filters[bitSet] = newAlloc;
        }
        self->filters[bitSet][countPresent++] = filter;

        // Set the counter properly again
        self->filterCounts[bitSet] = ((u64)countAlloc << 32) | (countPresent);
        //     DPRINTF(DEBUG_LVL_VVERB, "Setting final counter for %ld to 0x%lx\n",
        //            bitSet, self->filterCounts[bitSet]);
    }
    // Do the same thing for bit 0. Only do it once so we only free things once
    countPresent = (u32)(self->filterCounts[0] & 0xFFFFFFFF);
    countAlloc = (u32)(self->filterCounts[0] >> 32);
    if(countAlloc <= countPresent) {
        // Allocate using an exponential model
        if(countAlloc)
            countAlloc *= 2;
        else
            countAlloc = 1;
        ocrStatsFilter_t **newAlloc = (ocrStatsFilter_t**)malloc(sizeof(ocrStatsFilter_t*)*countAlloc);
        // TODO: This memcpy needs to be replaced. The malloc above as well...
        memcpy(newAlloc, self->filters[0], countPresent*sizeof(ocrStatsFilter_t*));
        if(countAlloc != 1)
            free(self->filters[0]);
        self->filters[0] = newAlloc;
    }
    self->filters[0][countPresent++] = filter;

    // Set the counter properly again
    self->filterCounts[0] = ((u64)countAlloc << 32) | (countPresent);

}

u8 intProcessMessage(ocrStatsProcess_t *dst) {
    ocrStatsMessage_t* msg = (ocrStatsMessage_t*)
        dst->messages->fctPtrs->popHead(dst->messages);

    if(msg) {
        DPRINTF(DEBUG_LVL_VERB, "Processing message 0x%lx for 0x%lx\n",
                (u64)msg, dst->me);
        u64 newTick = msg->tick > (dst->tick + 1)?msg->tick:(dst->tick + 1);
        msg->tick = dst->tick = newTick;
        DPRINTF(DEBUG_LVL_VVERB, "Message tick is %ld\n", msg->tick);

        u32 type = (u32)msg->type;
        u32 countFilter = dst->filterCounts[type] & 0xFFFFFFFF;
        if(countFilter) {
            // We have at least one filter that is registered to
            // this message type
            ocrStatsFilter_t **myFilters =  dst->filters[type];
            while(countFilter-- > 0) {
                DPRINTF(DEBUG_LVL_VVERB, "Sending message 0x%lx to filter 0x%lx\n",
                        (u64)msg, (u64)myFilters[countFilter]);
                myFilters[countFilter]->fcts.notify(myFilters[countFilter], msg);
            }
        }
        if(msg->state == 1) {
            msg->state = 2;
        } else {
            ASSERT(msg->state == 0);
            msg->fcts.destruct(msg);
        }
        return 1;
    } else {
        return 0;
    }
}
#endif /* OCR_ENABLE_STATISTICS */




