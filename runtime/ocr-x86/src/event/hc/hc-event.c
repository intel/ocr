/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_EVENT_HC

#include HAL_FILE
#include "debug.h"
#include "event/hc/hc-event.h"
#include "ocr-datablock.h"
#include "ocr-edt.h"
#include "ocr-event.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-task.h"
#include "utils/ocr-utils.h"
#include "ocr-worker.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

#define SEALED_LIST ((void *) -1)
#define END_OF_LIST NULL
#define UNINITIALIZED_DATA ((ocrGuid_t) -2)

#define DEBUG_TYPE EVENT

// REC: Copied from hc-task.c

// Internal signal/wait notification
// Signal a sticky event some data arrived (on its unique slot)
static void singleEventSignaled(ocrEvent_t * self, ocrGuid_t data, int slot) {
    // Single event signaled by another entity, call its satisfy method.
    self->fctPtrs->satisfy(self, data, slot);
}

// Internal signal/wait notification
// Signal a latch event some data arrived on one of its
static void latchEventSignaled(ocrEvent_t * self, ocrGuid_t data, int slot) {
    // latch event signaled by another entity, call its satisfy method.
    self->fctPtrs->satisfy(self, data, slot);
}

/******************************************************/
/* Signal/Wait interface implementation               */
/******************************************************/

// Registers a 'waiter' that should be signaled by 'self' on a particular slot.
// If 'self' has already been satisfied, it signals 'waiter' right away.
// Warning: Concurrent with  event's put.
// REC: This should move to hc-event.c as part of registerDependence
static void awaitableEventRegisterWaiter(ocrEventHcAwaitable_t * self, ocrGuid_t waiter, int slot) {
    // Try to insert 'waiter' at the beginning of the list
    regNode_t * curHead = (regNode_t *) self->waiters;
    if(curHead != SEALED_LIST) {
        regNode_t * newHead = checkedMalloc(newHead, sizeof(regNode_t));
        newHead->guid = waiter;
        newHead->slot = slot;
        newHead->next = (regNode_t *) curHead;
        while(curHead != SEALED_LIST && !__sync_bool_compare_and_swap(&(self->waiters), curHead, newHead)) {
            curHead = (regNode_t *) self->waiters;
            newHead->next = (regNode_t *) curHead;
        }
        if (curHead != SEALED_LIST) {
            // Insertion successful, we're done
            DPRINTF(DEBUG_LVL_INFO, "AddDependence from 0x%lx to 0x%lx slot %d\n", (((ocrEvent_t*)self)->guid), waiter, slot);
            return;
        }
        //else list has been sealed by a concurrent satisfy
        //need to reclaim non-inserted node
        free(newHead);
    }
    // Either the event was satisfied to begin with
    // or while we were trying to insert the waiter,
    // the event has been satisfied.
    signalWaiter(waiter, self->data, slot);
}

// Registers an edt to a once event by incrementing an atomic counter
// The event is deallocated only after being satisfied and all waiters 
// have been notified
static void onceEventRegisterEdtWaiter(ocrEvent_t * self, ocrGuid_t waiter, int slot) {
    ocrEventHcOnce_t* onceImpl = (ocrEventHcOnce_t*) self;
    DPRINTF(DEBUG_LVL_INFO, "Increment ONCE event reference %lx \n", self->guid);
    onceImpl->nbEdtRegistered->fctPtrs->xadd(onceImpl->nbEdtRegistered, 1);
}

//These are essentially switches to dispatch call to the correct implementation

void registerDependence(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot) {
    // Warning: signalerGuid can actually be a NULL_GUID.
    // This can happen when users declared a certain number of
    // depc but dynamically decide some are not used.

    // WAIT MODE: event-to-event registration
    // Note: do not call 'registerWaiter' here as it triggers event-to-edt
    // registration, which should only be done on edtSchedule.
    ocrPolicyDomain_t *pd = getCurrentPD();
    if (isEventGuid(signalerGuid) && isEventGuid(waiterGuid)) {
        ocrEventHcAwaitable_t * target;
        deguidify(pd, signalerGuid, (u64*)&target, NULL);
        awaitableEventRegisterWaiter(target, waiterGuid, slot);
        return;
    }
    // ONCE event must know who are consuming them so that 
    // they are not deallocated prematurely 
    if (isEventGuidOfKind(signalerGuid, OCR_EVENT_ONCE_T) && isEdtGuid(waiterGuid)) {
        ocrEvent_t * signalerEvent;
        deguidify(pd, signalerGuid, (u64*)&signalerEvent, NULL);
        onceEventRegisterEdtWaiter(signalerEvent, waiterGuid, slot);
    }

    // SIGNAL MODE:
    //  - anything-to-edt registration
    //  - db-to-event registration
    // Make sure to do this before signaling in case
    // the registerSignaler causes the waiter to fire
#ifdef OCR_ENABLE_STATISTICS
    statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif    
    registerSignaler(signalerGuid, waiterGuid, slot);
}

// Registers a waiter on a signaler
void registerWaiter(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot) {
    if(NULL_GUID == signalerGuid) {
        // If the dependence was a NULL_GUID, consider this slot is already satisfied
        signalWaiter(waiterGuid, NULL_GUID, slot);
    } else if (isEventGuid(signalerGuid)) {
        ASSERT(isEdtGuid(waiterGuid) || isEventGuid(waiterGuid));
        ocrEventHcAwaitable_t * target;
        deguidify(getCurrentPD(), signalerGuid, (u64*)&target, NULL);
        awaitableEventRegisterWaiter(target, waiterGuid, slot);
    } else if(isDatablockGuid(signalerGuid) && isEdtGuid(waiterGuid)) {
            signalWaiter(waiterGuid, signalerGuid, slot);
    } else {
        // Everything else is an error
        ASSERT("error: Unsupported guid kind in registerWaiter" );
    }
}

// register a signaler on a waiter
void registerSignaler(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot) {
    // anything to edt registration
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, NULL);
    if (isEdtGuid(waiterGuid)) {
        // edt waiting for a signal from an event or a datablock
        ASSERT((signalerGuid == NULL_GUID) || isEventGuid(signalerGuid) || isDatablockGuid(signalerGuid));
        ocrTask_t * target = NULL;

        deguidify(getCurrentPD(), waiterGuid, (u64*)&target, NULL);
        edtRegisterSignaler(target, signalerGuid, slot);
        if (target->depc == pd->getSys(pd)->xadd64(pd->getSys(pd),
                                                   &(target->addedDepCounter), 1)) {
            // This function pointer is called once, when all the dependence have been added
            target->fctPtrs->schedule(target);
        }
        return;
    // datablock to event registration => satisfy on the spot
    } else if (isDatablockGuid(signalerGuid)) {
        ASSERT(isEventGuid(waiterGuid));
        ocrEvent_t * target = NULL;
        deguidify(getCurrentPD(), waiterGuid, (u64*)&target, NULL);
        // This looks a duplicate of signalWaiter, however there hasn't
        // been any signal strictly speaking, hence calling satisfy directly
        ASSERT(isEventSingleGuid(waiterGuid) || isEventLatchGuid(waiterGuid));
        target->fctPtrs->satisfy(target, signalerGuid, slot);
        return;
    }
    // Remaining legal registrations is event-to-event, but this is
    // handled in 'registerWaiter'
    ASSERT(isEventGuid(waiterGuid) && isEventGuid(signalerGuid) && "error: Unsupported guid kind in registerDependence");
}

// signal a 'waiter' data has arrived on a particular slot
void signalWaiter(ocrGuid_t waiterGuid, ocrGuid_t data, int slot) {
    // TODO do we need to know who's signaling ?
    if (isEventSingleGuid(waiterGuid)) {
        ocrEvent_t * target = NULL;
        deguidify(getCurrentPD(), waiterGuid, (u64*)&target, NULL);
        singleEventSignaled(target, data, slot);
    } else if (isEventLatchGuid(waiterGuid)) {
        ocrEvent_t * target = NULL;
        deguidify(getCurrentPD(), waiterGuid, (u64*)&target, NULL);
        latchEventSignaled(target, data, slot);
    } else if(isEdtGuid(waiterGuid)) {
        ocrTask_t * target = NULL;
        deguidify(getCurrentPD(), waiterGuid, (u64*)&target, NULL);
        taskSignaled(target, data, slot);
    } else {
        // ERROR
        ASSERT(0 && "error: Unsupported guid kind in signal");
    }
}


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
    while(hal_cmpswap64(&(self->waiters), waiters, SEALED_LIST) != waiters) {
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
static void singleEventSatisfy(ocrEvent_t * base, ocrFatGuid_t data, u32 slotEvent) {
    ocrEventHcAwaitable_t * self = (ocrEventHcAwaitable_t *) base;
#ifdef OCR_ENABLE_STATISTICS
    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrGuid_t edt = getCurrentEDT();
    statsDEP_SATISFYToEvt(pd, edt, NULL, base->guid, base, data, slotEvent);
#endif
    
    // Whether it is a once, idem or sticky, unitialized means it's the first
    // time we try to satisfy the event. Note: It's a very loose check, the
    // 'Put' implementation must do more work to detect races on data.
    if (self->data.guid == UNINITIALIZED_DATA) {
        DPRINTF(DEBUG_LVL_INFO, "Satisfy %s: 0x%lx with 0x%lx\n", eventTypeToString(base), base->guid, data.guid);
        // Single events don't have slots, just put the data
        regNode_t * waiters = singleEventPut(self, data.guid);
        // Put must have sealed the waiters list and returned it
        // Note: Here the design is not great, 'put' CAS and returns the
        //       waiters list but does not handle the signaler list which
        //       is ok for now because it's not used.
        ASSERT(waiters != SEALED_LIST);
        ASSERT(self->data.guid != UNINITIALIZED_DATA);
        // Need to signal other entities waiting on the event
        regNode_t * waiter = waiters;
        while (waiter != END_OF_LIST) {
            // Note: in general it's better to read from self->data in case
            //       the event does some post processing on the input data
#ifdef OCR_ENABLE_STATISTICS
            // We do this *before* calling signalWaiter because otherwise
            // if the event is a OCR_EVENT_ONCE_T, it will get freed
            // before we can get the message sent
            statsDEP_SATISFYFromEvt(pd, base->guid, base, waiter->guid,
                                    data.guid, waiter->slot);
#endif
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

static ocrFatGuid_t singleEventGet (ocrEvent_t * base, u32 slot) {
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
#ifdef OCR_ENABLE_STATISTICS
    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrGuid_t edt = getCurrentEDT();
    statsDEP_SATISFYToEvt(pd, edt, NULL, base->guid, base, data, slot);
#endif
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
#ifdef OCR_ENABLE_STATISTICS
            statsDEP_SATISFYFromEvt(pd, base->guid, base, waiter->guid,
                                    data, waiter->slot);
#endif
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
#ifdef OCR_ENABLE_STATISTICS
    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrGuid_t edt = getCurrentEDT();
    statsDEP_SATISFYToEvt(pd, edt, NULL, base->guid, base, data, slot);
#endif
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
#ifdef OCR_ENABLE_STATISTICS
            statsDEP_SATISFYFromEvt(pd, base->guid, base, outputEventWaiter->guid,
                                    self->returnGuid, outputEventWaiter->slot);
#endif
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
        base->fcts->destruct(base);
    }
}

// Finish-latch event doesn't have an output value
static ocrGuid_t finishLatchEventGet(ocrEvent_t * base, u32 slot) {
    return NULL_GUID;
}

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
    base->fcts = eventFctPtrs;
    base->guid = UNINITIALIZED_GUID;
    guidify(pd, (u64)base, &(base->guid), OCR_GUID_EVENT);
    DPRINTF(DEBUG_LVL_INFO, "Create %s: 0x%lx\n", eventTypeToString(base), base->guid);
    return base;
}

void destructEventHc ( ocrEvent_t* base ) {
    // Event's signaler/waiter must have been previously deallocated
    // at some point before. For instance on satisfy.
    DPRINTF(DEBUG_LVL_INFO, "Destroy %s: 0x%lx\n", eventTypeToString(base), base->guid);
    ocrEventHc_t* derived = (ocrEventHc_t*)base;
    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrPolicyCtx_t * orgCtx = getCurrentWorkerContext();
    ocrPolicyCtx_t * ctx = orgCtx->clone(orgCtx);
#ifdef OCR_ENABLE_STATISTICS
    statsEVT_DESTROY(pd, getCurrentEDT(), NULL, base->guid, base);
#endif
    ctx->type = PD_MSG_GUID_REL;
    pd->inform(pd, base->guid, ctx);
    if(base->kind == OCR_EVENT_ONCE_T) {
        ocrEventHcOnce_t * onceEvent = (ocrEventHcOnce_t *) base;
        onceEvent->nbEdtRegistered->fcts->destruct(onceEvent->nbEdtRegistered);
    }
    ctx->destruct(ctx);
    free(derived);
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
#ifdef OCR_ENABLE_STATISTICS
    statsEVT_CREATE(getCurrentPD(), getCurrentEDT(), NULL, res->guid, res);
#endif
    return res;
}

static void destructEventFactoryHc ( ocrEventFactory_t * base ) {
    runtimeChunkFree((u64)base, NULL);
}

ocrEventFactory_t * newEventFactoryHc(ocrParamList_t *perType, u32 factoryId) {
    ocrEventFactoryHc_t* derived = (ocrEventFactoryHc_t*) runtimeChunkAlloc(
        sizeof(ocrEventFactoryHc_t), NULL);
    ocrEventFactory_t* base = (ocrEventFactory_t*) derived;
    
    base->instantiate = newEventHc;
    base->destruct =  destructEventFactoryHc;
    // initialize singleton instance that carries hc implementation function pointers
    base->fcts.destruct = &destructEventHc;
    base->fcts.get = &getEventHc;
    base->fcts.satisfy = &satisfyEventHc;
    base->fcts.registerDependence = &registerDependenceEventHc;
    base->factoryId = factoryId;

    return base;
}
#endif /* ENABLE_EVENT_HC */
