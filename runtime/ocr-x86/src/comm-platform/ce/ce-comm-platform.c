/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_CE

#include "debug.h"

#include "ocr-comp-platform.h"
#include "ocr-policy-domain.h"

#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"

#include "ce-comm-platform.h"

#include "mmio-table.h"
#include "rmd-arch.h"
#include "rmd-map.h"
#include "rmd-msg-queue.h"
#include "rmd-mmio.h"

#define DEBUG_TYPE COMM_PLATFORM

//
// Hgh-Level Theory of Operation / Design
//
// Communication will always involve one local->remote copy of
// information. Whether it is a source initiated bulk DMA or a series
// of remote initiated loads, it *will* happen. What does *not* need
// to happen are any additional local copies between the caller and
// callee on the sending end.
//
// Hence:
//
// (a) Every XE has a local receive stage. Every CE has per-XE receive
//     stage.  All receive stages are at MSG_QUEUE_OFFT in the agent
//     scratchpad and are MSG_QUEUE_SIZE bytes.
//
// (b) Every receive stage starts with an F/E word, followed by
//     content.
//
// (c) ceCommSendMessage():
//
//     CEs do not initiate communication, they only respond. Hence,
//     Send() ops do not expect a reply (they *are* replies
//     themselves) and so they will always be synchronous and once
//     data has been shipped out the buffer passed into the Send() is
//     free to be reused.
//
//        - Atomically test & set remote stage to F. Error if already F.
//        - DMA to remote stage
//        - Send() returns.
//
//     NOTE: XE software needs to consume a response from its stage
//           before injecting another request to CE. Otherwise, there
//           is the possibility of a race in the likely case that the
//           netowrk & CE are much faster than an XE...
//
// (d) ceCommPollMessage() -- non-blocking receive
//
//     Check local stage's F/E word. If E, return empty. If F, return content.
//
// (e) ceCommWaitMessage() -- blocking receive
//
//     While local stage E, keep looping. (FIXME: rate-limit w/ timer?)
//     Once it is F, return content.
//
// (f) ceCommDestructMessage() -- callback to notify received message was consumed
//
//     Atomically test & set local stage to E. Error if already E.
//

void ceCommDestruct (ocrCommPlatform_t * base) {

    runtimeChunkFree((u64)base, NULL);
}

void ceCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t * api) {

    u64 i;

    ASSERT(commPlatform != NULL && PD != NULL && api != NULL);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)commPlatform;

    // FIXME: HACK!!! HACK!!! HACK!!!
    // Because currently PD->Start() never returns, the CE cannot
    // Start() before booting its XEs. So, it boots the XEs and
    // Start()s only then. Which leads to a race between XEs Send()ing
    // to the CE and the CE initializing its comm-platform. The comm
    // buffers need to be cleared before use (otherwise you get
    // Full/Empty bit issues), but if we clear them here, we may clear
    // the first message sent by a fast XE that was started before us.
    // The HACK FIX is to clear the CE message buffers in RMDKRNL
    // before OCR on both CEs and XEs is started, so we shouldn't
    // clear it here now (commented out below.) Eventually, when the
    // Begin()/Start() issues are resolved and we can init properly
    // this HACK needs to be reversed...
    //
    // Zero-out our stage for receiving messages
    //for(i=MSG_QUEUE_OFFT; i<(MAX_NUM_XE * MSG_QUEUE_SIZE); i += sizeof(u64))
    //    *(volatile u64 *)i = 0;

    // Fill-in location tuples: ours and our parent's (the CE in FSIM)
    PD->myLocation = (ocrLocation_t)rmd_ld64(CE_MSR_BASE + CORE_LOCATION * sizeof(u64));
    hal_fence();
    PD->parentLocation = PD->myLocation; /* FIXME: Currently will work only for single block, but a hierarchy someday??? */

    // Remember our PD in case we need to call through it later
    cp->pdPtr = PD;

    // Pre-compute pointer to our block's XEs' remote stages (where we send to)
    // Pre-compute pointer to our block's XEs' local stages (where they send to us)
    for(i=0; i<MAX_NUM_XE ; i++) {
        cp->rq[i] = (u64 *)(BR_XE_BASE(i) + MSG_QUEUE_OFFT);
        cp->lq[i] = (u64 *)((u64)MSG_QUEUE_OFFT + i * MSG_QUEUE_SIZE);
    }

    // Arbitrary first choice
    cp->pollq = 0;

    // Statically check stage area is big enough for 1 policy message + F/E word
    COMPILE_TIME_ASSERT(MSG_QUEUE_SIZE >= (sizeof(u64) + sizeof(ocrPolicyMsg_t)));
}

void ceCommStart(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t * api) {

    ASSERT(commPlatform != NULL && PD != NULL && api != NULL);
}

void ceCommStop(ocrCommPlatform_t * commPlatform) {

    ASSERT(commPlatform != NULL);
}

void ceCommFinish(ocrCommPlatform_t *commPlatform) {

    int i;

    ASSERT(commPlatform != NULL);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)commPlatform;

    // Clear settings
    cp->pdPtr->myLocation = cp->pdPtr->parentLocation = (ocrLocation_t)0x0;
    cp->pdPtr = NULL;
    for(i=0; i<MAX_NUM_XE; i++) {
        cp->rq[i] = NULL;
        cp->lq[i] = NULL;
    }
    cp->pollq = 0;
}

u8 ceCommSetMaxExpectedMessageSize(ocrCommPlatform_t *self, u64 size, u32 mask) {

    ASSERT(0);
    return 0;
}

u8 ceCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target,
                     ocrPolicyMsg_t *message, u64 bufferSize, u64 *id,
                     u32 properties, u32 mask) {

    ASSERT(self != NULL);
    ASSERT(message != NULL);
    ASSERT(bufferSize != 0);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)self;

    // TODO: compute all-but-agent & compare between us & target
    // Target XE's stage (note this is in remote XE memory!)
    u64 * rq = cp->rq[((u64)target) & ID_AGENT_MASK];

    // bufferSize: Top u32 is buffer size, bottom u32 is message size

    // - Check remote stage Empty/Busy/Full is Empty.
    {
        u64 tmp = rmd_ld64((u64)rq);
        ASSERT(tmp == 0);
    }

#ifndef ENABLE_BUILDER_ONLY
    // - DMA to remote stage
    // We marshall things properly
    u64 fullMsgSize = 0, marshalledSize = 0;
    ocrCommPlatformGetMsgSize(message, &fullMsgSize, &marshalledSize);
    // We can only deal with the case where everything fits in the message
    if(fullMsgSize > (bufferSize >> 32)) {
        DPRINTF(DEBUG_LVL_WARN, "Comm platform only handles messages up to size %ld\n",
                bufferSize >> 32);
        ASSERT(0);
    }
    ocrCommPlatformMarshallMsg(message, (u8*)message, MARSHALL_APPEND);
    // - DMA to remote stage, with fence
    DPRINTF(DEBUG_LVL_VVERB, "DMA-ing out message to 0x%lx of size %d\n",
            (u64)&rq[1], message->size);
    rmd_mmio_dma_copyregion_async((u64)message, (u64)&rq[1], message->size);

    // - Fence DMA
    rmd_fence_fbm();
    // - Atomically test & set remote stage to Full. Error if already non-Empty.
    {
        u64 tmp = rmd_mmio_xchg64((u64)rq, (u64)2);
        ASSERT(tmp == 0);
    }
#endif

    return 0;
}

u8 ceCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                     u64* bufferSize, u32 properties, u32 *mask) {

    ASSERT(self != NULL);
    ASSERT(msg != NULL);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)self;

    // Local stage is at well-known 0x0
    u64 i = cp->pollq;

    // Check local stage's F/E word. If E, return empty. If F, return content.

    // Loop once over the local stages searching for a message
    do {
        // Check this stage for Empty/Busy/Full
        if((cp->lq[i])[0] == 2) {
            // Have one, break out
            break;
        } else {
            // Advance to next queue
            i = (i+1) % MAX_NUM_XE;
        }
    } while(i != cp->pollq);

    // Either we got a message in the current queue, or we ran through all of them and they're all empty
    if((cp->lq[i])[0] == 2) {
        // We have a message
        // Provide a ptr to the local stage's contents
        *msg = (ocrPolicyMsg_t *)&((cp->lq[i])[1]);
        // We fixup pointers
        u64 fullMsgSize = 0, marshalledSize = 0;
        ocrCommPlatformGetMsgSize(*msg, &fullMsgSize, &marshalledSize);
        if(fullMsgSize > sizeof(ocrPolicyMsg_t)) {
            DPRINTF(DEBUG_LVL_WARN, "Comm platform only handles messages up to size %ld\n",
                    sizeof(ocrPolicyMsg_t));
            ASSERT(0);
        }
        (*msg)->size = fullMsgSize; // Reset it properly
        ocrCommPlatformUnMarshallMsg((u8*)*msg, NULL, *msg, MARSHALL_APPEND);

        // Advance queue for next check
        cp->pollq = (i + 1) % MAX_NUM_XE;

        // One message being returned
        return 0;
    } else {
        // We ran through all of them and they're all empty
        return POLL_NO_MESSAGE;
    }
}

u8 ceCommDestructMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t *msg);

u8 ceCommWaitMessage(ocrCommPlatform_t *self,  ocrPolicyMsg_t **msg,
                     u64* bufferSize, u32 properties, u32 *mask) {

    ASSERT(self != NULL);
    ASSERT(msg != NULL);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)self;

    // Local stage is at well-known 0x0
    u64 i;

    DPRINTF(DEBUG_LVL_VVERB, "Going to wait for message (starting at %d)\n",
            cp->pollq);
    // Loop throught the stages till we receive something
    for(i = cp->pollq; (cp->lq[i])[0] != 2; i = (i+1) % MAX_NUM_XE) {
        // Halt the CPU, instead of burning rubber
        // An alarm would wake us, so no delay will result
        // TODO: REC: Loop around at least once if we get
        // an alarm to check all slots.
        // Note that a timer alarm wakes us up periodically
        __asm__ __volatile__("hlt\n\t");
    }

#if 1
    // We have a message
    DPRINTF(DEBUG_LVL_VVERB, "Waited for message and got message from %d at 0x%lx\n",
            i, &((cp->lq[i])[1]));
    *msg = (ocrPolicyMsg_t *)&((cp->lq[i])[1]);
    // We fixup pointers
    u64 fullMsgSize = 0, marshalledSize = 0;
    ocrCommPlatformGetMsgSize(*msg, &fullMsgSize, &marshalledSize);
    if(fullMsgSize > sizeof(ocrPolicyMsg_t)) {
        DPRINTF(DEBUG_LVL_WARN, "Comm platform only handles messages up to size %ld\n",
                sizeof(ocrPolicyMsg_t));
        ASSERT(0);
    }
    (*msg)->size = fullMsgSize; // Reset it properly
    ocrCommPlatformUnMarshallMsg((u8*)*msg, NULL, *msg, MARSHALL_APPEND);
#else
    // We have a message
    // NOTE: For now we copy it into the buffer provided by the caller
    //       eventually when QMA arrives we'll move to a posted-buffer
    //       scheme and eliminate the copies.
    hal_memCopy(*msg, &((cp->lq[i])[1]), sizeof(ocrPolicyMsg_t), 0);
    hal_fence();
#endif

    // Set queue index to this queue, so we'll know what to "destruct" later...
    cp->pollq = i;

    // One message being returned
    return 0;
}

u8 ceCommDestructMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t *msg) {

    ASSERT(self != NULL);
    ASSERT(msg != NULL);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)self;
    // Remember XE number
    u64 n = cp->pollq;

    DPRINTF(DEBUG_LVL_VVERB, "Destructing message received from %d (un-clock gate)\n", n);

    // Sanity check we're "destruct"ing the right thing...
    ASSERT(msg == (ocrPolicyMsg_t *)&((cp->lq[cp->pollq])[1]));

    // "Free" stage
    (cp->lq[cp->pollq])[0] = 0;

    // Advance queue for next time
    cp->pollq = (cp->pollq + 1) % MAX_NUM_XE;

    {
        // Clear the XE pipeline clock gate while preserving other bits.
        u64 state = rmd_ld64(XE_MSR_BASE(n) + (FUB_CLOCK_CTL * sizeof(u64)));
        rmd_st64_async( XE_MSR_BASE(n) + (FUB_CLOCK_CTL * sizeof(u64)), state & ~0x10000000ULL );
    }

    return 0;
}

ocrCommPlatform_t* newCommPlatformCe(ocrCommPlatformFactory_t *factory,
                                     ocrParamList_t *perInstance) {

    ocrCommPlatformCe_t * commPlatformCe = (ocrCommPlatformCe_t*)
                                           runtimeChunkAlloc(sizeof(ocrCommPlatformCe_t), PERSISTENT_CHUNK);
    ocrCommPlatform_t * derived = (ocrCommPlatform_t *) commPlatformCe;
    factory->initialize(factory, derived, perInstance);
    return derived;
}

void initializeCommPlatformCe(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * derived, ocrParamList_t * perInstance) {
    initializeCommPlatformOcr(factory, derived, perInstance);
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

void destructCommPlatformFactoryCe(ocrCommPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCommPlatformFactory_t *newCommPlatformFactoryCe(ocrParamList_t *perType) {
    ocrCommPlatformFactory_t *base = (ocrCommPlatformFactory_t*)
                                     runtimeChunkAlloc(sizeof(ocrCommPlatformFactoryCe_t), NONPERSISTENT_CHUNK);

    base->instantiate = &newCommPlatformCe;
    base->initialize = &initializeCommPlatformCe;
    base->destruct = &destructCommPlatformFactoryCe;

    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), ceCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*, ocrCommApi_t *),
                                         ceCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*, ocrCommApi_t *),
                                         ceCommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), ceCommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), ceCommFinish);
    base->platformFcts.setMaxExpectedMessageSize = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, u64, u32),
            ceCommSetMaxExpectedMessageSize);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrLocation_t,
                                     ocrPolicyMsg_t *, u64, u64*, u32, u32), ceCommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**, u64*, u32, u32*),
                                     ceCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t**, u64*, u32, u32*),
                                     ceCommWaitMessage);
    base->platformFcts.destructMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*, ocrPolicyMsg_t*),
                                         ceCommDestructMessage);

    return base;
}
#endif /* ENABLE_COMM_PLATFORM_CE */
