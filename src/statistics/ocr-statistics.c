/**
 * @brief OCR implementation of statistics gathering
 * @authors Romain Cledat, Intel Corporation
 * @date 2013-04-03
 * Copyright (c) 2013, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifdef OCR_ENABLE_STATISTICS

#include "ocr-statistics.h"
#include "ocr-config.h"
// TODO: Potentially have an ocr-driver.h that defines all the factories that
// would be setup in ocr-driver.c
//#include "ocr-driver.h"

// Forward declaration
static u8 intProcessMessage(ocrStatsProcess_t *dst);

void ocrStatsProcessCreate(ocrStatsProcess_t *self, ocrGuid_t guid) {
    u64 i;
    self->me = guid;
    self->processing = GocrLockFactory->instantiate(GocrLockFactory, NULL);
    self->messages = GocrQueueFactory->instantiate(GocrQueueFactory, (void*)32); // TODO: Make size configurable
    self->tick = 0;
    filters = (ocrStatsFilter_t***)malloc(sizeof(ocrStatsFilter_t**)*STATS_EVT_MAX);
    filterCounts = (u64*)malloc(sizeof(u64)*STATS_EVT_MAX);
    for(i = 0; i < (u64)STATS_EVT_MAX; ++i) filterCounts[i] = 0ULL;

    DO_DEBUG(DEBUG_LVL_INFO) {
        PRINTF("INFO: Created a StatsProcess (0x%lx) with GUID 0x%lx\n", (u64)self, guid.data);
    }
}

void ocrStatsProcessDestruct(ocrStatsProcess_t *self) {
    u64 i;
    u32 j;

    DO_DEBUG(DEBUG_LVL_INFO) {
        PRINTF("INFO: Destroying StatsProcess 0x%lx (GUID: 0x%lx)\n", (u64)self, self->me.data);
    }

    // Make sure we process all remaining messages
    ASSERT(self->processing->trylock(self->processing));

    while(intProcessMessage(self)) ;

    // Free all the filters associated with this process
    // Assumes that the filter is only associated with ONE
    // process
    for(i = 0; i < (u64)STATS_EVT_MAX; ++i) {
        u32 maxJ = (u32)(filterCounts[i] & 0xFFFFFFFF);
        for(j = 0; j < maxJ; ++j) {
            free(filters[i][j]);
        }
        free(filters[i]);
    }

    free(filters);
    free(filterCounts);
    self->processing->destruct(self->processing);
    self->messages->destruct(self->messages);
}

void ocrStatsProcessRegisterFilter(ocrStatsProcess_t *self, u64 bitMask,
                                   ocrStatsFilter_t *filter) {

    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: Registering filter 0x%lx with process 0x%lx for mask 0x%lx\n",
               (u64)filter, (u64)self, bitMask);
    }
    while(bitMask) {
        // Setting some filter up
        u64 bitSet = fls64(bitMask);
        bitMask &= ~(1ULL << bitSet);

        ASSERT(bitSet < STATS_EVT_MAX);

        // Check to make sure we have enough room to add this filter
        u32 countAlloc, countPresent;
        countPresent = (u32)(filterCounts[bitSet] & 0xFFFFFFFF);
        countAlloc = (u32)(filterCounts[bitSet] >> 32);
        DO_DEBUG(DEBUG_LVL_VVERB) {
            PRINTF("VVERB: For type %ld, have %d present and %d allocated (from 0x%lx)\n",
                   bitSet, countPresent, countAlloc, filterCounts[bitSet]);
        }
        if(countAlloc <= countPresent) {
            // Allocate using an exponential model
            if(countAlloc)
                countAlloc *= 2;
            else
                countAlloc = 1;
            ocrStatsFilter_t **newAlloc = (ocrStatsFilter_t**)malloc(sizeof(ocrStatsFilter_t*)*countAlloc);
            // TODO: This memcpy needs to be replaced. The malloc above as well...
            memcpy(newAlloc, filters[bitSet], countPresent*sizeof(ocrStatsFilter_t*));
            free(filters[bitSet]);
            filters[bitSet] = newAlloc;
        }
        filters[bitSet][countPresent++] = filter;

        // Set the counter properly again
        filterCounts[bitSet] = (countAlloc << 32) | (countPresent);
        DO_DEBUG(DEBUG_LVL_VVERB) {
            PRINTF("VVERB: Setting final counter for %ld to 0x%lx\n",
                   bitSet, filterCounts[bitSet]);
        }
    }
}

void ocrStatsAsyncMessage(ocrStatsProcess_t *src, ocrStatsProcess_t *dst,
                          ocrStatsMessage_t *msg) {


    u64 tickVal = (src->tick += 1);
    DO_DEBUG(DEBUG_LVL_VVERB) {
        PRINTF("VERB: Message from 0x%lx with timestamp %ld\n", src->me.data, tickVal);
    }
    msg->tick = tickVal;
    msg->state = 0;
    ASSERT(msg->src.data == src->me.data);
    ASSERT(msg->dest.data == dst->me.data);

    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: Message 0x%lx -> 0x%lx of type %d (0x%lx)\n",
               src->me.data, dst->me.data, (int)msg->type, (u64)msg);
    }
    // Push the message into the messages queue
    dst->messages->pushTail(dst->messages, (u64)msg);

    // Now try to get the lock on processing
    if(dst->processing->trylock(dst->processing)) {
        // We grabbed the lock
        DO_DEBUG(DEBUG_LVL_VERB) {
            PRINTF("VERB: Message 0x%lx -> 0x%lx: grabbing processing lock\n", src->me.data, dst->me.data);
        }
        u32 count = 5; // TODO: Make configurable
        while(count-- > 0) {
            if(!intProcessMessage(dst))
                break;
        }
        DO_DEBUG(DEBUG_LVL_VERB) {
            PRINTF("VERB: Finished processing messages for 0x%lx\n", dst->me.data);
        }
    }
}

void ocrStatsSyncMessage(ocrStatsProcess_t *src, ocrStatsProcess_t *dst,
                          ocrStatsMessage_t *msg) {

    u64 tickVal = (src->tick += 1);
    DO_DEBUG(DEBUG_LVL_VVERB) {
        PRINTF("VERB: SYNC Message from 0x%lx with timestamp %ld\n", src->me.data, tickVal);
    }
    msg->tick = tickVal;
    msg->state = 1;
    ASSERT(msg->src.data == src->me.data);
    ASSERT(msg->dest.data == dst->me.data);

    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: SYNC Message 0x%lx -> 0x%lx of type %d (0x%lx)\n",
               src->me.data, dst->me.data, (int)msg->type, (u64)msg);
    }
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
            DO_DEBUG(DEBUG_LVL_VERB) {
                PRINTF("VERB: Message 0x%lx -> 0x%lx: grabbing processing lock\n", src->me.data, dst->me.data);
            }
            s32 count = 5; // TODO: Make configurable
            // Process at least count and at least until we get to our message
            // EXTREMELY RARE PROBLEM of count running over for REALLY deep queues
            while(count-- > 0 || msg->state != 2) {
                if(!intProcessMessage(dst) && (msg->state == 2))
                    break;
            }
            DO_DEBUG(DEBUG_LVL_VERB) {
                PRINTF("VERB: Finished processing messages for 0x%lx\n", dst->me.data);
            }
            break; // Break out of while(1)
        } else {
            // Backoff (TODO, need HW support (abstraction though))
        }
    }

    // At this point, we can sync our own tick
    ASSERT(msg->state == 2);
    ASSERT(msg->tick >= src->tick);
    src->tick = msg->tick;
    DO_DEBUG(DEBUG_LVL_VVERB) {
        PRINTF("VVERB: Sync tick out: %ld\n", src->tick);
    }
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
    ocrMessage_t* msg = (ocrMessage_t*)popHead(dst->messages);

    if(msg) {
        DO_DEBUG(DEBUG_LVL_VERB) {
            PRINTF("VERB: Processing message 0x%lx for 0x%lx\n",
                   (u64)msg, dst->me.data);
        }
        u64 newTick = msg->tick > (dst->tick + 1)?msg->tick:(dst->tick + 1);
        msg->tick = dst->tick = newTick;
        DO_DEBUG(DEBUG_LVL_VVERB) {
            PRINTF("VVERB: Message tick is %ld\n", msg->tick);
        }

        u32 type = (u32)msg->type;
        u32 countFilter = dst->filterCounts[type] & 0xFFFFFFFF;
        if(countFilter) {
            // We have at least one filter that is registered to
            // this message type
            ocrStatsFilter_t **myFilters =  dst->filters[type];
            while(countFilter-- > 0) {
                DO_DEBUG(DEBUG_LVL_VVERB) {
                    PRINTF("VVERB: Sending message 0x%lx to filter 0x%lx\n",
                           (u64)msg, (u64)myFilters[countFilter]);
                }
                myFilters[countFilter]->notify(myFilters[countFilter], msg);
            }
        }
        if(msg->state == 1) {
            msg->state = 2;
        } else {
            ASSERT(msg->state == 0);
            msg->destruct(msg);
        }
    } else {
        return 0;
    }
}
#endif /* OCR_ENABLE_STATISTICS */
