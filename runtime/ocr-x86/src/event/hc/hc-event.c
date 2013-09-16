/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "debug.h"
#include "event/hc/hc-event.h"
#include "ocr-datablock.h"
#include "ocr-edt.h"
#include "ocr-event.h"
#include "ocr-macros.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-task.h"
#include "ocr-utils.h"
#include "ocr-worker.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#define SEALED_LIST ((void *) -1)
#define END_OF_LIST NULL
#define UNINITIALIZED_DATA ((ocrGuid_t) -2)

#define DEBUG_TYPE EVENT

/******************************************************/
/* OCR-HC Debug                                       */
/******************************************************/

static char * eventTypeToString(ocrEvent_t * base) {
    ocrEventTypes_t type = base->kind;
    if(type == OCR_EVENT_ONCE_T) {
        return "once";
    } else if (type == OCR_EVENT_IDEM_T) {
        return "idem";
    } else if (type == OCR_EVENT_STICKY_T) {
        return "sticky";
    } else if (type == OCR_EVENT_LATCH_T) {
        return "latch";
    } else if (type == OCR_EVENT_FINISH_LATCH_T) {
        return "finish-latch";
    } else {
        return "unknown";
    }
}

//
// Convenience function to get the current EDT and deguidify it
//
static inline ocrTask_t * getCurrentTask() {
    ocrGuid_t edtGuid = getCurrentEDT();
    // the bootstrap process launching mainEdt returns NULL_GUID for the current EDT
    if (edtGuid != NULL_GUID) {
        ocrTask_t * task = NULL;
        deguidify(getCurrentPD(), edtGuid, (u64*)&task, NULL);
        return task;
    }
    return NULL;
}

//
// forward declarations
//
extern void signalWaiter(ocrGuid_t waiterGuid, ocrGuid_t data, u32 slot);
extern void setFinishLatch(ocrTask_t * edt, ocrGuid_t latchGuid);
static void finishLatchCheckout(ocrEvent_t * base);


/******************************************************/
/* OCR-HC Events Implementation                       */
/******************************************************/

//
// OCR-HC Events Life-Cycle
//

// Generic initializer for events. Implementation-dependent event functions
// are passed through the function pointer data-structure.
static ocrEvent_t* eventConstructorInternal(ocrPolicyDomain_t * pd, ocrEventFactory_t * factory, ocrEventTypes_t eventType, bool takesArg) {
    ocrEvent_t* base = NULL;
    ocrEventFcts_t * eventFctPtrs = NULL;
    if (eventType == OCR_EVENT_FINISH_LATCH_T) {
        ocrEventHcFinishLatch_t * eventImpl = (ocrEventHcFinishLatch_t*) checkedMalloc(eventImpl, sizeof(ocrEventHcFinishLatch_t));
        eventImpl->counter = 0;
        //Note: waiters are initialized afterwards
        eventFctPtrs = &(((ocrEventFactoryHc_t*)factory)->finishLatchFcts);
        base = (ocrEvent_t*)eventImpl;
    } else if (eventType == OCR_EVENT_LATCH_T) {
        ocrEventHcLatch_t * eventImpl = (ocrEventHcLatch_t*) checkedMalloc(eventImpl, sizeof(ocrEventHcLatch_t));
        (eventImpl->base).waiters = END_OF_LIST;
        (eventImpl->base).signalers = END_OF_LIST;
        (eventImpl->base).data = NULL_GUID;
        eventImpl->counter = 0;
        eventFctPtrs = &(factory->latchFcts);
        base = (ocrEvent_t*)eventImpl;
    } else {
        ASSERT(((eventType == OCR_EVENT_ONCE_T) ||
                (eventType == OCR_EVENT_IDEM_T) ||
                (eventType == OCR_EVENT_STICKY_T)) &&
                "error: Unsupported type of event");
        ocrEventHcSingle_t* eventImpl;
        if (eventType == OCR_EVENT_ONCE_T) {
            ocrEventHcOnce_t* onceImpl = (ocrEventHcOnce_t*) checkedMalloc(eventImpl, sizeof(ocrEventHcOnce_t));        
            onceImpl->nbEdtRegistered = pd->getAtomic64(pd, NULL);
            eventImpl = (ocrEventHcSingle_t*) onceImpl;
        } else {
            eventImpl = (ocrEventHcSingle_t*) checkedMalloc(eventImpl, sizeof(ocrEventHcSingle_t));
        }
        (eventImpl->base).waiters = END_OF_LIST;
        (eventImpl->base).signalers = END_OF_LIST;
        (eventImpl->base).data = UNINITIALIZED_DATA;
        eventFctPtrs = &(factory->singleFcts);
        base = (ocrEvent_t*)eventImpl;
    }

    // Initialize ocrEventHc_t
    base->kind = eventType;

    // Initialize ocrEvent_t base
    base->fctPtrs = eventFctPtrs;
    base->guid = UNINITIALIZED_GUID;
    guidify(pd, (u64)base, &(base->guid), OCR_GUID_EVENT);
    DPRINTF(DEBUG_LVL_INFO, "Create %s: 0x%lx\n", eventTypeToString(base), base->guid);
    return base;
}

static void destructEventHc ( ocrEvent_t* base ) {
    // Event's signaler/waiter must have been previously deallocated
    // at some point before. For instance on satisfy.
    DPRINTF(DEBUG_LVL_INFO, "Destroy %s: 0x%lx\n", eventTypeToString(base), base->guid);
    ocrEventHc_t* derived = (ocrEventHc_t*)base;
    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrPolicyCtx_t * orgCtx = getCurrentWorkerContext();
    ocrPolicyCtx_t * ctx = orgCtx->clone(orgCtx);
    ctx->type = PD_MSG_GUID_REL;
    pd->inform(pd, base->guid, ctx);
    if(base->kind == OCR_EVENT_ONCE_T) {
        ocrEventHcOnce_t * onceEvent = (ocrEventHcOnce_t *) base;
        onceEvent->nbEdtRegistered->fctPtrs->destruct(onceEvent->nbEdtRegistered);
    }
    ctx->destruct(ctx);
    free(derived);
}


//
// OCR-HC Single Events Implementation
//

// Set a single event data guid
// Warning: Concurrent with registration on sticky event.
static regNode_t * singleEventPut(ocrEventHcAwaitable_t * self, ocrGuid_t data ) {
    ASSERT (data != UNINITIALIZED_DATA && "error: put UNINITIALIZED_DATA");
    // TODO This is not enough to always detect concurrent puts.
    ASSERT (self->data == UNINITIALIZED_DATA && "violated single assignment property for EDFs");
    self->data = data;
    // list of entities waiting on the event
    volatile regNode_t* waiters = self->waiters;
    // 'put' may be concurrent to another entity trying to register to this event
    // Try to cas the waiters list to SEALED_LIST to let other know the 'put' has occurred.
    while ( !__sync_bool_compare_and_swap( &(self->waiters), waiters, SEALED_LIST)) {
        // If we failed to cas, it means a new node has been inserted
        // in front of the registration list, so update our pointer and try again.
        waiters = self->waiters;
    }
    // return the most up to date head on the registration list
    // (no more adds possible once SEALED_LIST has been set)
    ASSERT(self->waiters == SEALED_LIST);
    return (regNode_t *) waiters;
}

// This is setting a sticky event's data
// slotEvent is ignored for stickies.
static void singleEventSatisfy(ocrEvent_t * base, ocrGuid_t data, u32 slotEvent) {
    ocrEventHcAwaitable_t * self = (ocrEventHcAwaitable_t *) base;

    // Whether it is a once, idem or sticky, unitialized means it's the first
    // time we try to satisfy the event. Note: It's a very loose check, the
    // 'Put' implementation must do more work to detect races on data.
    if (self->data == UNINITIALIZED_DATA) {
        DPRINTF(DEBUG_LVL_INFO, "Satisfy %s: 0x%lx with 0x%lx\n", eventTypeToString(base), base->guid, data);
        // Single events don't have slots, just put the data
        regNode_t * waiters = singleEventPut(self, data);
        // Put must have sealed the waiters list and returned it
        // Note: Here the design is not great, 'put' CAS and returns the
        //       waiters list but does not handle the signaler list which
        //       is ok for now because it's not used.
        ASSERT(waiters != SEALED_LIST);
        ASSERT(self->data != UNINITIALIZED_DATA);
        // Need to signal other entities waiting on the event
        regNode_t * waiter = waiters;
        while (waiter != END_OF_LIST) {
            // Note: in general it's better to read from self->data in case
            //       the event does some post processing on the input data
            signalWaiter(waiter->guid, self->data, waiter->slot);
            waiters = waiter;
            waiter = waiter->next;
            free(waiters); // Release waiter node
        }
    } else {
        // once-events cannot survive down here
        // idem-events ignore extra satisfy
        if (base->kind == OCR_EVENT_STICKY_T) {
            ASSERT(false && "error: Multiple satisfy on a sticky event");
        }
    }
}

static ocrGuid_t singleEventGet (ocrEvent_t * base, u32 slot) {
    ocrEventHcAwaitable_t * self = (ocrEventHcAwaitable_t*) base;
    return (self->data == UNINITIALIZED_DATA) ? ERROR_GUID : self->data;
}


//
// OCR-HC Latch Events Implementation
//


// Runs concurrently with registration on latch
static void latchEventSatisfy(ocrEvent_t * base, ocrGuid_t data, u32 slot) {
    ASSERT((slot == OCR_EVENT_LATCH_DECR_SLOT) || (slot == OCR_EVENT_LATCH_INCR_SLOT));
    ocrEventHcLatch_t * latch = (ocrEventHcLatch_t *) base;
    u32 incr = (slot == OCR_EVENT_LATCH_DECR_SLOT) ? -1 : 1;
    u32 count;
    do {
        count = latch->counter;
    } while(!__sync_bool_compare_and_swap(&(latch->counter), count, count+incr));

    DPRINTF(DEBUG_LVL_INFO, "Satisfy %s: 0x%lx %s\n", eventTypeToString(base), base->guid, ((slot == OCR_EVENT_LATCH_DECR_SLOT) ? "decr":"incr"));
    if ((count+incr) == 0) {
        DPRINTF(DEBUG_LVL_INFO, "Satisfy %s: 0x%lx reached zero\n", eventTypeToString(base), base->guid);
        ocrEventHcAwaitable_t * self = (ocrEventHcAwaitable_t *) base;
        //TODO add API to seal a list
        //TODO: do we need volatile here ?
        // CAS the wait-list
        volatile regNode_t* waiters = self->waiters;
        // satisfy is concurrent with registration on the event
        // Try to cas the waiters list to SEALED_LIST to let others know the
        // latch has been satisfied
        while ( !__sync_bool_compare_and_swap( &(self->waiters), waiters, SEALED_LIST)) {
            // If we failed to cas, it means a new node has been inserted
            // in front of the registration list, so update our pointer and try again.
            waiters = self->waiters;
        }
        ASSERT(self->waiters == SEALED_LIST);
        regNode_t * waiter = (regNode_t*) waiters;
        // Go over the list and notify waiters
        while (waiter != END_OF_LIST) {
            // For now latch events don't have any data output
            signalWaiter(waiter->guid, NULL_GUID, waiter->slot);
            waiters = waiter;
            waiter = waiter->next;
            free((regNode_t*) waiters); // Release waiter node
        }
    }
}

// Latch-event doesn't have an output value
static ocrGuid_t latchEventGet(ocrEvent_t * base, u32 slot) {
    return NULL_GUID;
}


//
// OCR-HC Finish-Latch Events Implementation
//

// Requirements:
//  R1) All dependences (what the finish latch will satisfy) are provided at creation. This implementation DOES NOT support outstanding registrations.
//  R2) Number of incr and decr signaled on the event MUST BE equal.
static void finishLatchEventSatisfy(ocrEvent_t * base, ocrGuid_t data, u32 slot) {
    ASSERT((slot == OCR_EVENT_LATCH_DECR_SLOT) || (slot == OCR_EVENT_LATCH_INCR_SLOT));
    ocrEventHcFinishLatch_t * self = (ocrEventHcFinishLatch_t *) base;
    u32 incr = (slot == OCR_EVENT_LATCH_DECR_SLOT) ? -1 : 1;
    u32 count;
    do {
        count = self->counter;
    } while(!__sync_bool_compare_and_swap(&(self->counter), count, count+incr));
    DPRINTF(DEBUG_LVL_VERB, "Satisfy %s: 0x%lx %s\n", eventTypeToString(base), base->guid, ((slot == OCR_EVENT_LATCH_DECR_SLOT)? "decr":"incr"));
    // No possible race when we reached 0 (see R2)
    if ((count+incr) == 0) {
        DPRINTF(DEBUG_LVL_INFO, "Satisfy %s: 0x%lx reached zero\n", eventTypeToString(base), base->guid);
        // Important to void the ELS at that point, to make sure there's no
        // side effect on code executing downwards.
        ocrTask_t * task = getCurrentTask();
        setFinishLatch(task, NULL_GUID);
        // Notify waiters the latch is satisfied (We can extend that to a list // of waiters if we need to. (see R1))
        // Notify output event if any associated with the finish-edt
        regNode_t * outputEventWaiter = &(self->outputEventWaiter);
        if (outputEventWaiter->guid != NULL_GUID) {
            signalWaiter(outputEventWaiter->guid, self->returnGuid, outputEventWaiter->slot);
        }
        // Notify the parent latch if any
        regNode_t * parentLatchWaiter = &(self->parentLatchWaiter);
        if (parentLatchWaiter->guid != NULL_GUID) {
            ocrEvent_t * parentLatch;
            deguidify(getCurrentPD(), parentLatchWaiter->guid, (u64*)&parentLatch, NULL);
            finishLatchCheckout(parentLatch);
        }
        // Since finish-latch is internal to finish-edt, and ELS is cleared,
        // there are no more pointers left to it, deallocate.
        base->fctPtrs->destruct(base);
    }
}

// Finish-latch event doesn't have an output value
static ocrGuid_t finishLatchEventGet(ocrEvent_t * base, u32 slot) {
    return NULL_GUID;
}


/******************************************************/
/* OCR-HC Finish-Latch Utilities                      */
/******************************************************/

// satisfies the incr slot of a finish latch event

// satisfies the decr slot of a finish latch event. Deallocate the latch
// event if satisfied and set 'base' to NULL.
static void finishLatchCheckout(ocrEvent_t * base) {
    finishLatchEventSatisfy(base, NULL_GUID, OCR_EVENT_LATCH_DECR_SLOT);
}

/******************************************************/
/* OCR-HC Events Factory                              */
/******************************************************/

// takesArg indicates whether or not this event carries any data
static ocrEvent_t * newEventHc ( ocrEventFactory_t * factory, ocrEventTypes_t eventType,
                                 bool takesArg, ocrParamList_t *perInstance) {
    ocrEvent_t * res = eventConstructorInternal(getCurrentPD(), factory, eventType, takesArg);
    return res;
}

static void destructEventFactoryHc ( ocrEventFactory_t * base ) {
     free(base);
}

ocrEventFactory_t * newEventFactoryHc(ocrParamList_t *perType) {
    ocrEventFactoryHc_t* derived = (ocrEventFactoryHc_t*) checkedMalloc(derived, sizeof(ocrEventFactoryHc_t));
    ocrEventFactory_t* base = (ocrEventFactory_t*) derived;
    base->instantiate = newEventHc;
    base->destruct =  destructEventFactoryHc;
    // initialize singleton instance that carries hc implementation function pointers
    base->singleFcts.destruct = destructEventHc;
    base->singleFcts.get = singleEventGet;
    base->singleFcts.satisfy = singleEventSatisfy;

    // latch-events
    base->latchFcts.destruct = destructEventHc;
    base->latchFcts.get = latchEventGet;
    base->latchFcts.satisfy = latchEventSatisfy;

    //Note: Just store finish-latch function ptrs in a static since this is
    //      runtime implementation specific
    derived->finishLatchFcts.destruct = destructEventHc;
    derived->finishLatchFcts.get = finishLatchEventGet;
    derived->finishLatchFcts.satisfy = finishLatchEventSatisfy;

    return base;
}
