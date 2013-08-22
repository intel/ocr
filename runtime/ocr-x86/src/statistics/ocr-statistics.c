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
#include "ocr-statistics.h"
#include "ocr-types.h"


#define DEBUG_TYPE STATS
// Forward declaration
extern u8 intProcessMessage(ocrStatsProcess_t *dst);


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
    dst->messages->fctPtrs->pushTail(dst->messages, (u64)msg);

    // Now try to get the lock on processing
    if(dst->processing->fctPtrs->trylock(dst->processing)) {
        // We grabbed the lock
        DPRINTF(DEBUG_LVL_VERB, "Message 0x%lx -> 0x%lx: grabbing processing lock\n", src->me, dst->me);
        u32 count = 5; // TODO: Make configurable
        while(count-- > 0) {
            if(!intProcessMessage(dst))
                break;
        }
        DPRINTF(DEBUG_LVL_VERB, "Finished processing messages for 0x%lx\n", dst->me);
        // Unlock
        dst->processing->fctPtrs->unlock(dst->processing);
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
    dst->messages->fctPtrs->pushTail(dst->messages, (u64)msg);

    // Now try to get the lock on processing
    // Since this is a sync message, we need to make sure
    // we don't come out of this without having had our message
    // processed. We are going to repeatedly try to grab the lock
    // with a backoff and sit around until we either process our
    // message or someone else does it
    while(1) {
        if(dst->processing->fctPtrs->trylock(dst->processing)) {
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
            dst->processing->fctPtrs->unlock(dst->processing);

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
    msg->fcts.destruct(msg);
}

#endif /* OCR_ENABLE_STATISTICS */


