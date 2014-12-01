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

// Ugly globals below, but would go away once FSim has QMA support trac #232
#define OUTSTANDING_CE_MSGS 4
u64 msgAddresses[OUTSTANDING_CE_MSGS] = {0, 0, 0, 0};
ocrPolicyMsg_t sendBuf, recvBuf; // Currently size of 1 msg each.
// TODO: The above need to be placed in the CE's scratchpad, but once QMAs are ready, should
// be no problem. trac #232

u8 ceCommCheckCEMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg);
u8 ceCommDestructMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t *msg);
u8 ceCommDestructCEMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t *msg);

void ceCommDestruct (ocrCommPlatform_t * base) {

    runtimeChunkFree((u64)base, NULL);
}

void ceCommBegin(ocrCommPlatform_t * commPlatform, ocrPolicyDomain_t * PD, ocrCommApi_t * api) {

    u64 i;

    ASSERT(commPlatform != NULL && PD != NULL && api != NULL);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)commPlatform;
    commPlatform->pd = PD;

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
    commPlatform->location = PD->myLocation;
    hal_fence();
    // My parent is my unit's block 0 CE
    PD->parentLocation = (PD->myLocation & ~(ID_BLOCK_MASK|ID_AGENT_MASK)) | ID_AGENT_CE; // My parent is my unit's block 0 CE
    // If I'm a block 0 CE, my parent is unit 0 block 0 CE
    if ((PD->myLocation & ID_BLOCK_MASK) == 0)
        PD->parentLocation = (PD->myLocation & ~(ID_UNIT_MASK|ID_BLOCK_MASK|ID_AGENT_MASK))
                             | ID_AGENT_CE;
    // TODO: Generalize this to cover higher levels of hierarchy too. trac #231

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
    u32 i;
    ASSERT(commPlatform != NULL);
    for(i=0; i< OUTSTANDING_CE_MSGS; i++) {
        ((ocrPolicyMsg_t *)msgAddresses[i])->type = 0xdead;
        msgAddresses[i] = 0xdead;
    }
    recvBuf.type = 0;
    sendBuf.type = 0;
}

void ceCommFinish(ocrCommPlatform_t *commPlatform) {

    u32 i;

    ASSERT(commPlatform != NULL);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)commPlatform;

    // Clear settings
    cp->pdPtr->myLocation = cp->pdPtr->parentLocation = (ocrLocation_t)0x0;
    commPlatform->location = (ocrLocation_t)0x0;
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

u8 ceCommSendMessageToCE(ocrCommPlatform_t *self, ocrLocation_t target,
                     ocrPolicyMsg_t *message, u64 bufferSize, u64 *id,
                     u32 properties, u32 mask) {

    u32 i;
    u64 msgAbsAddr;
    static u64 msgId = 0xace00000000;         // Known signature to aid grepping
    if(msgId==0xace00000000)
        msgId |= (self->location & 0xFF)<<16; // Embed the src location onto msgId
    ASSERT(self->location != target);

    // If this is not a blocking message, return if busy
    if(!(properties & BLOCKING_SEND_MSG_PROP) && sendBuf.type) {
        if(sendBuf.type == 0xdead) sendBuf.type = 0;
        return 1;
    }

    u64 fullMsgSize = 0, marshalledSize = 0;
    ocrPolicyMsgGetMsgSize(message, &fullMsgSize, &marshalledSize, 0);
    if(fullMsgSize > (bufferSize >> 32)) {
        DPRINTF(DEBUG_LVL_WARN, "Comm platform only handles messages up to size %ld\n",
                bufferSize >> 32);
        ASSERT(0);
    }

    message->msgId = msgId++;
    message->type |= PD_CE_CE_MESSAGE;
    ocrPolicyMsgMarshallMsg(message, (u8 *)&sendBuf, MARSHALL_FULL_COPY);

    msgAbsAddr = DR_CE_BASE(CHIP_FROM_ID(self->location),
                            UNIT_FROM_ID(self->location), BLOCK_FROM_ID(self->location))
                            + (u64)(&sendBuf);

    u64 *rmbox = (u64 *) (DR_CE_BASE(CHIP_FROM_ID(target),
                                   UNIT_FROM_ID(target), BLOCK_FROM_ID(target))
                                   + (u64)(msgAddresses));
    u32 k = 0;
    do {
        for(i = 0; i<OUTSTANDING_CE_MSGS; i++, k++) {
            if(hal_cmpswap64(&rmbox[i], 0, msgAbsAddr) == 0) break;
        }
    } while((i == OUTSTANDING_CE_MSGS) && ((k<10000)));
    // TODO: This value is arbitrary.
    // What is a reasonable number of tries before giving up?

    // FIXME: Receiver is busy, we need to cancel & retry later.
    // Any other ways to get around this?
    if(i==OUTSTANDING_CE_MSGS) {
        sendBuf.type = 0;
        DPRINTF(DEBUG_LVL_INFO, "Cancel send msg %lx type %lx, properties %x; (%lx->%lx)\n",
                                 message->msgId, message->type, properties, self->location,
                                 target);
        if(rmbox[0] == 0xdead) return 2; // Shutdown in progress - no retry
        return 1;                        // otherwise, retry send
    }

    DPRINTF(DEBUG_LVL_VVERB, "Sent msg %lx type %lx; (%lx->%lx)\n", message->msgId, message->type, self->location, target);
    return 0;
}

u8 ceCommSendMessage(ocrCommPlatform_t *self, ocrLocation_t target,
                     ocrPolicyMsg_t *message, u64 bufferSize, u64 *id,
                     u32 properties, u32 mask) {

    ASSERT(self != NULL);
    ASSERT(message != NULL);
    ASSERT(bufferSize != 0);
    ASSERT(target != self->location);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)self;

    // If target is not in the same block, use a different function
    // FIXME: do the same for chip/unit/board as well, or better yet, a new macro
    if((self->location & ~ID_AGENT_MASK) != (target & ~ID_AGENT_MASK)) {
        return ceCommSendMessageToCE(self, target, message, bufferSize, id, properties, mask);
    } else {

        // TODO: compute all-but-agent & compare between us & target
        // Target XE's stage (note this is in remote XE memory!)
        u64 * rq = cp->rq[((u64)target) & ID_AGENT_MASK];

        // bufferSize: Top u32 is buffer size, bottom u32 is message size

        // - Check remote stage Empty/Busy/Full is Empty.
        {
            u64 tmp = rmd_ld64((u64)rq);
            if(tmp) return 1; // Temporary workaround till bug #134 is fixed
//            ASSERT(tmp == 0);
        }

#ifndef ENABLE_BUILDER_ONLY
        // - DMA to remote stage
        // We marshall things properly
        u64 fullMsgSize = 0, marshalledSize = 0;
        ocrPolicyMsgGetMsgSize(message, &fullMsgSize, &marshalledSize, 0);
        // We can only deal with the case where everything fits in the message
        if(fullMsgSize > (bufferSize >> 32)) {
            DPRINTF(DEBUG_LVL_WARN, "Comm platform only handles messages up to size %ld\n",
                bufferSize >> 32);
            ASSERT(0);
        }
        ocrPolicyMsgMarshallMsg(message, (u8*)message, MARSHALL_APPEND);
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
}

u8 ceCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                     u64* bufferSize, u32 properties, u32 *mask) {

    ASSERT(self != NULL);
    ASSERT(msg != NULL);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)self;

    if(properties & PD_CE_CE_MESSAGE) { // Poll only for CE messages
        return ceCommCheckCEMessage(self, msg);
    } else ASSERT(0); // Nothing else supported

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
        ocrPolicyMsgGetMsgSize(*msg, &fullMsgSize, &marshalledSize, 0);
        if(fullMsgSize > sizeof(ocrPolicyMsg_t)) {
            DPRINTF(DEBUG_LVL_WARN, "Comm platform only handles messages up to size %ld\n",
                    sizeof(ocrPolicyMsg_t));
            ASSERT(0);
        }
        (*msg)->size = fullMsgSize; // Reset it properly
        ocrPolicyMsgUnMarshallMsg((u8*)*msg, NULL, *msg, MARSHALL_APPEND);

        // Advance queue for next check
        cp->pollq = (i + 1) % MAX_NUM_XE;

        // One message being returned
        return 0;
    } else {
        // We ran through all of them and they're all empty
        return POLL_NO_MESSAGE;
    }
}

u8 ceCommCheckCEMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg) {
    u32 j;

    // Go through our mbox to check for valid messages
    for(j=0; j<OUTSTANDING_CE_MSGS; j++)
    //for(j=OUTSTANDING_CE_MSGS-1; j >=0; j--)
        if(msgAddresses[j])
            break;

    // Found a valid message, process it
    if(j<OUTSTANDING_CE_MSGS) {
        ASSERT(msgAddresses[j]);
        *msg = (ocrPolicyMsg_t *)(msgAddresses[j]);
        // We fixup pointers
        u64 fullMsgSize = 0, marshalledSize = 0;

        ocrPolicyMsgGetMsgSize(*msg, &fullMsgSize, &marshalledSize, 0);
        if(fullMsgSize > sizeof(ocrPolicyMsg_t)) {
            DPRINTF(DEBUG_LVL_WARN, "Comm platform only handles messages up to size %ld\n",
                    sizeof(ocrPolicyMsg_t));
            ASSERT(0);
        }
        DPRINTF(DEBUG_LVL_VVERB, "Got message %lx from CE of type %x\n", (*msg)->msgId, (*msg)->type);
        (*msg)->size = fullMsgSize; // Reset it properly
        ocrPolicyMsgUnMarshallMsg((u8*)*msg, NULL, &recvBuf, MARSHALL_FULL_COPY);
        ceCommDestructCEMessage(self, *msg);
        *msg = &recvBuf;
        return 0;
    }
    return 1;
}

u8 ceCommWaitMessage(ocrCommPlatform_t *self,  ocrPolicyMsg_t **msg,
                     u64* bufferSize, u32 properties, u32 *mask) {

    ASSERT(self != NULL);
    ASSERT(msg != NULL);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)self;

    // Give priority to CE messages
    if(!ceCommCheckCEMessage(self, msg)) return 0;

    // Local stage is at well-known 0x0
    u32 i, j;

    DPRINTF(DEBUG_LVL_VVERB, "Going to wait for message (starting at %d)\n",
            cp->pollq);
    // Loop throught the stages till we receive something
    for(i = cp->pollq, j=(cp->pollq - 1 + MAX_NUM_XE) % MAX_NUM_XE;
           (cp->lq[i])[0] != 2; i = (i+1) % MAX_NUM_XE) {
        // Halt the CPU, instead of burning rubber
        // An alarm would wake us, so no delay will result
        // TODO: REC: Loop around at least once if we get
        // an alarm to check all slots.
        // Note that a timer alarm wakes us up periodically
        if(i==j) {
             // Before going to sleep, check for CE messages
             if(!ceCommCheckCEMessage(self, msg)) return 0;
             __asm__ __volatile__("hlt\n\t");
        }
    }

#if 1
    // We have a message
    DPRINTF(DEBUG_LVL_VVERB, "Waited for message and got message from %d at 0x%lx\n",
            i, &((cp->lq[i])[1]));
    *msg = (ocrPolicyMsg_t *)&((cp->lq[i])[1]);
    // We fixup pointers
    u64 fullMsgSize = 0, marshalledSize = 0;
    ocrPolicyMsgGetMsgSize(*msg, &fullMsgSize, &marshalledSize, 0);
    if(fullMsgSize > sizeof(ocrPolicyMsg_t)) {
        DPRINTF(DEBUG_LVL_WARN, "Comm platform only handles messages up to size %ld\n",
                sizeof(ocrPolicyMsg_t));
        ASSERT(0);
    }
    (*msg)->size = fullMsgSize; // Reset it properly
    ocrPolicyMsgUnMarshallMsg((u8*)*msg, NULL, *msg, MARSHALL_APPEND);
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

u8 ceCommDestructCEMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t *msg) {
   u32 i;

   for(i = 0; i<OUTSTANDING_CE_MSGS; i++)
       if(msgAddresses[i] == (u64)msg)
           break;

   if(i<OUTSTANDING_CE_MSGS) {
       DPRINTF(DEBUG_LVL_VVERB, "Destructing msg %lx\n", msg->msgId);
       msg->type = 0;
       msgAddresses[i] = 0;
   }

   return 0;
}

u8 ceCommDestructMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t *msg) {

    ASSERT(self != NULL);
    ASSERT(msg != NULL);

    ocrCommPlatformCe_t * cp = (ocrCommPlatformCe_t *)self;

    if(msg->type == 0) return 0; // FIXME: This is needed to ignore shutdown
                                 // messages. To go away once #134 is fixed
    if(msg->type & PD_CE_CE_MESSAGE) {
        if (msg->destLocation == self->location)
            return ceCommDestructCEMessage(self, msg);
        else
            return 0;
    }

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
