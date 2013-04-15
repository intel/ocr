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
#include "ocr-driver.h"

void ocrStatsProcessCreate(ocrStatsProcess_t *self, ocrGuid_t guid) {
    u64 i;
    self->me = guid;
    self->tick = GocrLockFactory->instantiate(NULL);
    filters = (ocrStatsFilter_t***)malloc(sizeof(ocrStatsFilter_t**)*STATS_EVT_MAX);
    filterCounts = (u64*)malloc(sizeof(u64)*STATS_EVT_MAX);
    for(i = 0; i < (u64)STATS_EVT_MAX; ++i) filterCounts[i] = 0ULL;
}

void ocrStatsProcessDestruct(ocrStatsProcess_t *self) {
    u64 i;
    u32 j;

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
    self->tick->destruct(self->tick);
}

void ocrStatsProcessRegisterFilter(ocrStatsProcess_t *self, u64 bitMask,
                                   ocrStatsFilter_t *filter) {
    while(bitMask) {
        // Setting some filter up
        u64 bitSet = fls64(bitMask);
        bitMask &= ~(1ULL << bitSet);

        ASSERT(bitSet < STATS_EVT_MAX);

        // Check to make sure we have enough room to add this filter
        u32 countAlloc, countPresent;
        countPresent = (u32)(filterCounts[bitSet] & 0xFFFFFFFF);
        countAlloc = (u32)(filterCounts[bitSet] >> 32);
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
    }
}

void ocrStatsAsyncMessage(ocrStatsProcess_t *src, ocrStatsProcess_t *dst,
                          ocrStatsMessage_t *msg) {


    u64 tickVal = src->tick->xadd(src->tick, 1);

    u64 tickValCmp, tickValOld, tickValMax;
    u64 tickValCmp = src->tick->val(src->tick);

    // Atomically set the dst tick
    do {
        tickValOld = tickValCmp;
        tickValMax = tickVal>tickValOld?tickVal:(tickValOld+1);
        tickValCmp = dst->tick->cmpswap(dst->tick, tickValOld, tickValMax);
    } while(tickValCmp != tickValOld);

    msg->tick = tickValMax;
    intStatsSendToFilter(dst, msg);
}

void ocrStatsSyncMessage(ocrStatsProcess_t *src, ocrStatsProcess_t *dst,
                          ocrStatsMessage_t *msg) {

    ocrStatsAsyncMessage(src, dst, msg);
    ASSERT(dst->tick >= src->tick);
    src->tick = dst->tick;
}

/* Internal methods */

/**
 * @brief Process a message received and send it off to the
 * relevant filters
 *
 * This function will distribute the received message to the
 * relevant filter(s).
 */
void intStatsSendToFilter(ocrStatsProcess_t *dst, ocrStatsMessage_t *msg) {

}
#endif /* OCR_ENABLE_STATISTICS */
