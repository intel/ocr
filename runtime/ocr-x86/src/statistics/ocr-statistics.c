/**
 * @brief OCR implementation of statistics gathering
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifdef OCR_ENABLE_STATISTICS

#include "debug.h"
#include "ocr-config.h"
#include "ocr-statistics.h"

#include <stdlib.h>
#include <string.h>
// TODO: Potentially have an ocr-driver.h that defines all the factories that
// would be setup in ocr-driver.c
//#include "ocr-driver.h"

#define DEBUG_TYPE STATS
// Forward declaration
static u8 intProcessMessage(ocrStatsProcess_t *dst);

// Message related functions
void ocrStatsMessageCreate(ocrStatsMessage_t *self, ocrStatsEvt_t type, u64 tick, ocrGuid_t src,
                           ocrGuid_t dest, void* configuration) {
    self->tick = tick;
    self->src = src;
    self->dest = dest;
    self->type = type;
    self->state = 0;
}

void ocrStatsMessageDestruct(ocrStatsMessage_t *self) {
    free(self);
}

// Process related functions
void ocrStatsProcessCreate(ocrStatsProcess_t *self, ocrGuid_t guid) {
    u64 i;
    self->me = guid;
    self->processing = GocrLockFactory->instantiate(GocrLockFactory, NULL);
    self->messages = GocrQueueFactory->instantiate(GocrQueueFactory, (void*)32); // TODO: Make size configurable
    self->tick = 0;
    self->filters = (ocrStatsFilter_t***)malloc(sizeof(ocrStatsFilter_t**)*(STATS_EVT_MAX+1)); // +1 because the first bucket keeps track of all filters uniquely
    self->filterCounts = (u64*)malloc(sizeof(u64)*(STATS_EVT_MAX+1));
    for(i = 0; i < (u64)STATS_EVT_MAX + 1; ++i) self->filterCounts[i] = 0ULL;

    DPRINTF(DEBUG_LVL_INFO, "Created a StatsProcess (0x%lx) for object with GUID 0x%lx\n", (u64)self, guid);
}

void ocrStatsProcessDestruct(ocrStatsProcess_t *self) {
    u64 i;
    u32 j;

    DPRINTF(DEBUG_LVL_INFO, "Destroying StatsProcess 0x%lx (GUID: 0x%lx)\n", (u64)self, self->me);

    // Make sure we process all remaining messages
    while(!self->processing->trylock(self->processing)) ;

    while(intProcessMessage(self)) ;

    // Free all the filters associated with this process
    // Assumes that the filter is only associated with ONE
    // process
    u32 maxJ = (u32)(self->filterCounts[0] & 0xFFFFFFFF);
    for(j = 0; j < maxJ; ++j) {
        self->filters[0][j]->destruct(self->filters[0][j]);
    }
    if(maxJ)
        free(self->filters[0]);
    for(i = 1; i < (u64)STATS_EVT_MAX + 1; ++i) {
        if(self->filterCounts[i])
            free(self->filters[i]);
    }

    free(self->filters);
    free(self->filterCounts);

    self->processing->unlock(self->processing);
    self->processing->destruct(self->processing);

    self->messages->destruct(self->messages);
}

void ocrStatsProcessRegisterFilter(ocrStatsProcess_t *self, u64 bitMask,
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

void ocrStatsAsyncMessage(ocrStatsProcess_t *src, ocrStatsProcess_t *dst,
                          ocrStatsMessage_t *msg) {


    u64 tickVal = (src->tick += 1);
    DPRINTF(DEBUG_LVL_VVERB, "Message from 0x%lx with timestamp %ld\n", src->me, tickVal);
    msg->tick = tickVal;
    msg->state = 0;
    ASSERT(msg->src == src->me);
    ASSERT(msg->dest == dst->me);

    DPRINTF(DEBUG_LVL_VERB, "Message 0x%lx -> 0x%lx of type %d (0x%lx)\n",
            src->me, dst->me, (int)msg->type, (u64)msg);
    // Push the message into the messages queue
    dst->messages->pushTail(dst->messages, (u64)msg);

    // Now try to get the lock on processing
    if(dst->processing->trylock(dst->processing)) {
        // We grabbed the lock
        DPRINTF(DEBUG_LVL_VERB, "Message 0x%lx -> 0x%lx: grabbing processing lock\n", src->me, dst->me);
        u32 count = 5; // TODO: Make configurable
        while(count-- > 0) {
            if(!intProcessMessage(dst))
                break;
        }
        DPRINTF(DEBUG_LVL_VERB, "Finished processing messages for 0x%lx\n", dst->me);
        // Unlock
        dst->processing->unlock(dst->processing);
    }
}

void ocrStatsSyncMessage(ocrStatsProcess_t *src, ocrStatsProcess_t *dst,
                          ocrStatsMessage_t *msg) {

    u64 tickVal = (src->tick += 1);
    DPRINTF(DEBUG_LVL_VVERB, "SYNC Message from 0x%lx with timestamp %ld\n", src->me, tickVal);
    msg->tick = tickVal;
    msg->state = 1;
    ASSERT(msg->src == src->me);
    ASSERT(msg->dest == dst->me);

    DPRINTF(DEBUG_LVL_VERB, "SYNC Message 0x%lx -> 0x%lx of type %d (0x%lx)\n",
            src->me, dst->me, (int)msg->type, (u64)msg);
    // Push the message into the messages queue
    dst->messages->pushTail(dst->messages, (u64)msg);

    // Now try to get the lock on processing
    // Since this is a sync message, we need to make sure
    // we don't come out of this without having had our message
    // processed. We are going to repeatedly try to grab the lock
    // with a backoff and sit around until we either process our
    // message or someone else does it
    while(1) {
        if(dst->processing->trylock(dst->processing)) {
            // We grabbed the lock
            DPRINTF(DEBUG_LVL_VERB, "Message 0x%lx -> 0x%lx: grabbing processing lock\n", src->me, dst->me);
            s32 count = 5; // TODO: Make configurable
            // Process at least count and at least until we get to our message
            // EXTREMELY RARE PROBLEM of count running over for REALLY deep queues
            while(count-- > 0 || msg->state != 2) {
                if(!intProcessMessage(dst) && (msg->state == 2))
                    break;
            }
            DPRINTF(DEBUG_LVL_VERB, "Finished processing messages for 0x%lx\n", dst->me);
            // Unlock
            dst->processing->unlock(dst->processing);

            break; // Break out of while(1)
        } else {
            // Backoff (TODO, need HW support (abstraction though))
        }
    }

    // At this point, we can sync our own tick
    ASSERT(msg->state == 2);
    ASSERT(msg->tick >= src->tick);
    src->tick = msg->tick;
    DPRINTF(DEBUG_LVL_VVERB, "Sync tick out: %ld\n", src->tick);
    msg->destruct(msg);
}

/* Internal methods */

/**
 * @brief Process a message from the head of dst
 *
 * Returns 0 if no messages on the queue and 1 if a
 * message was processed
 *
 * @param dst       ocrProcess_t to process messages for
 * @return 0 if no message processed and 1 otherwise
 * @warn This function is *not* thread-safe. Admission control
 * must ensure that only one worker calls this at any given point (although
 * multiple workers can call it over time)
 */
static u8 intProcessMessage(ocrStatsProcess_t *dst) {
    ocrStatsMessage_t* msg = (ocrStatsMessage_t*)dst->messages->popHead(dst->messages);

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
                myFilters[countFilter]->notify(myFilters[countFilter], msg);
            }
        }
        if(msg->state == 1) {
            msg->state = 2;
        } else {
            ASSERT(msg->state == 0);
            msg->destruct(msg);
        }
        return 1;
    } else {
        return 0;
    }
}
#endif /* OCR_ENABLE_STATISTICS */
