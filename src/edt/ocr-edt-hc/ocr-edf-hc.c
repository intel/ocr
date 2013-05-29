/* Copyright (c) 2012, Rice University

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1.  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2.  Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
   3.  Neither the name of Intel Corporation
   nor the names of its contributors may be used to endorse or
   promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdlib.h>
#include <assert.h>
#include "ocr-macros.h"
#include "hc_edf.h"
#include "ocr-datablock.h"
#include "ocr-utils.h"
#include "ocr-task-event.h"
#include "ocr-worker.h"
#include "debug.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

struct ocr_event_factory_struct* hc_event_factory_constructor(void) {
    hc_event_factory* derived = (hc_event_factory*) malloc(sizeof(hc_event_factory));
    ocr_event_factory* base = (ocr_event_factory*) derived;
    base->create = hc_event_factory_create;
    base->destruct =  hc_event_factory_destructor;
    return base;
}

static bool isEventGuid(ocrGuid_t guid) {
    ocrGuidKind kind;
    globalGuidProvider->getKind(globalGuidProvider, guid, &kind);
    return kind == OCR_GUID_EVENT;
}

static bool isEdtGuid(ocrGuid_t guid) {
    ocrGuidKind kind;
    globalGuidProvider->getKind(globalGuidProvider, guid, &kind);
    return kind == OCR_GUID_EDT;
}

static bool isEventLatchGuid(ocrGuid_t guid) {
    if(isEventGuid(guid)) {
        hc_event_t * event = NULL;
        globalGuidProvider->getVal(globalGuidProvider, guid, (u64*)&event, NULL);
        return (event->kind == OCR_EVENT_LATCH_T);
    }
    return false;
}

static bool isEventSingleGuid(ocrGuid_t guid) {
    if(isEventGuid(guid)) {
        hc_event_t * event = NULL;
        globalGuidProvider->getVal(globalGuidProvider, guid, (u64*)&event, NULL);
        return ((event->kind == OCR_EVENT_ONCE_T) 
            || (event->kind == OCR_EVENT_IDEM_T) 
            || (event->kind == OCR_EVENT_STICKY_T));
    }
    return false;
}


//
// EDT Properties Utilities
//

/**
   @brief return true if a property is part of a properties set
 **/
static bool hasProperty(u16 properties, u16 property) {
    return properties & property;
}


/******************************************************/
/* OCR-HC ELS Slots Declaration                       */
/******************************************************/

// This must be consistent with the ELS size the runtime is compiled with
#define ELS_SLOT_FINISH_LATCH 0

static ocrTask_t * getCurrentTask() {
    // TODO: we should be able to assert there must be an edt when we'll have a main OCR EDT
    ocrGuid_t edtGuid = getCurrentEDT();
    if (edtGuid != NULL_GUID) {
        ocrTask_t * event = NULL;
        globalGuidProvider->getVal(globalGuidProvider, edtGuid, (u64*)&event, NULL);
        return event;
    }
    return NULL;
}

//
// forward declarations
//

static void signalWaiter(ocrGuid_t waiterGuid, ocrGuid_t data, int slot);

static void registerSignaler(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot);

static void registerWaiter(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot);

// finish-latch function pointers
static ocr_event_fcts_t * event_fct_ptrs_finishlatch;

static void finishLatchCheckout(ocr_event_t * base);

/******************************************************/
/* OCR-HC Events Implementation                       */
/******************************************************/


// Define internal finish-latch event id after user-level events
#define OCR_EVENT_FINISH_LATCH_T OCR_EVENT_T_MAX+1

//
// OCR-HC Events Life-Cycle
//

// Generic initializer for events. Implementation-dependent event functions
// are passed through the function pointer data-structure.
static struct ocr_event_struct* eventConstructor(ocr_event_factory * factory, ocrEventTypes_t eventType, bool takesArg) {
    ocr_event_t* base = NULL;
    ocr_event_fcts_t * eventFctPtrs = NULL;
    if (eventType == OCR_EVENT_FINISH_LATCH_T) {
    	hc_event_finishlatch_t * eventImpl = (hc_event_finishlatch_t*) checked_malloc(eventImpl, sizeof(hc_event_finishlatch_t));
    	eventImpl->counter = 0;
    	//Note: waiters are initialized afterwards
    	eventFctPtrs = event_fct_ptrs_finishlatch;
        base = (ocr_event_t*)eventImpl;
    } else if (eventType == OCR_EVENT_LATCH_T) {
        hc_event_latch_t * eventImpl = (hc_event_latch_t*) checked_malloc(eventImpl, sizeof(hc_event_latch_t));
        (eventImpl->base).waiters = END_OF_LIST;
        (eventImpl->base).signalers = END_OF_LIST;
        (eventImpl->base).data = NULL_GUID;
        eventImpl->counter = 0;
        eventFctPtrs = factory->event_fct_ptrs_latch;
        base = (ocr_event_t*)eventImpl;
    } else {
        assert(((eventType == OCR_EVENT_ONCE_T) ||
                (eventType == OCR_EVENT_IDEM_T) ||
                (eventType == OCR_EVENT_STICKY_T)) &&
                "error: Unsupported type of event");
        hc_event_single_t* eventImpl = (hc_event_single_t*) checked_malloc(eventImpl, sizeof(hc_event_single_t));
        (eventImpl->base).waiters = END_OF_LIST;
        (eventImpl->base).signalers = END_OF_LIST;
        (eventImpl->base).data = UNINITIALIZED_DATA;
        eventFctPtrs = factory->event_fct_ptrs_single;
        base = (ocr_event_t*)eventImpl;
    }

    // Initialize hc_event_t
    hc_event_t * hcImpl =  (hc_event_t *) base;
    hcImpl->kind = eventType;

    // Initialize ocr_event_t base
    base->fct_ptrs = eventFctPtrs;
    base->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(base->guid), (u64)base, OCR_GUID_EVENT);

    return base;
}

void eventDestructor ( struct ocr_event_struct* base ) {
    // Event's signaler/waiter must have been deallocated at some point before.// For instance on satisfy.
    hc_event_t* derived = (hc_event_t*)base;
    globalGuidProvider->releaseGuid(globalGuidProvider, base->guid);
    free(derived);
}


//
// OCR-HC Single Events Implementation
//

// Registers a 'waiter' that should be signaled by 'self' on a particular slot.
// If 'self' has already been satisfied, it signals 'waiter' right away.
// Warning: Concurrent with  event's put.
static void awaitableEventRegisterWaiter(hc_event_awaitable_t * self, ocrGuid_t waiter, int slot) {
    // Try to insert 'waiter' at the beginning of the list
    reg_node_t * curHead = (reg_node_t *) self->waiters;
    if(curHead != SEALED_LIST) {
        reg_node_t * newHead = checked_malloc(newHead, sizeof(reg_node_t));
        newHead->guid = waiter;
        newHead->slot = slot;
        newHead->next = (reg_node_t *) curHead;
        while(curHead != SEALED_LIST && !__sync_bool_compare_and_swap(&(self->waiters), curHead, newHead)) {
            curHead = (reg_node_t *) self->waiters;
            newHead->next = (reg_node_t *) curHead;
        }
        if (curHead != SEALED_LIST) {
            // Insertion successful, we're done
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

// Set a single event data guid
// Warning: Concurrent with registration on sticky event.
static reg_node_t * singleEventPut(hc_event_awaitable_t * self, ocrGuid_t data ) {
    // TODO This is not enough to always detect concurrent puts.
    assert (self->data == UNINITIALIZED_DATA && "violated single assignment property for EDFs");
    self->data = data;
    // list of entities waiting on the event
    volatile reg_node_t* waiters = self->waiters;
    // 'put' may be concurrent to another entity trying to register to this event
    // Try to cas the waiters list to SEALED_LIST to let other know the 'put' has occurred.
    while ( !__sync_bool_compare_and_swap( &(self->waiters), waiters, SEALED_LIST)) {
        // If we failed to cas, it means a new node has been inserted
        // in front of the registration list, so update our pointer and try again.
        waiters = self->waiters;
    }
    // return the most up to date head on the registration list
    // (no more adds possible once SEALED_LIST has been set)
    assert(self->waiters == SEALED_LIST);
    return (reg_node_t *) waiters;
}

// This is setting a sticky event's data
// slotEvent is ignored for stickies.
void singleEventSatisfy(ocr_event_t * base, ocrGuid_t data, int slotEvent) {
    hc_event_t * hcSelf = (hc_event_t *) base;
    hc_event_awaitable_t * self = (hc_event_awaitable_t *) base;

    // Whether it is a once, idem or sticky, unitialized means it's the first
    // time we try to satisfy the event. Note: It's a very loose check, the 
    // 'Put' implementation must do more work to detect races on data.
    if (self->data == UNINITIALIZED_DATA) {
        // Single events don't have slots, just put the data
        reg_node_t * waiters = singleEventPut(self, data);
        // Put must have sealed the waiters list and returned it
        // Note: Here the design is not great, 'put' CAS and returns the 
        //       waiters list but does not handle the signaler list which 
        //       is ok for now because it's not used.
        assert(waiters != SEALED_LIST);
        assert(self->data != UNINITIALIZED_DATA);
        // Need to signal other entities waiting on the event
        reg_node_t * waiter = waiters;
        while (waiter != END_OF_LIST) {
            // Note: in general it's better to read from self->data in case
            //       the event does some post processing on the input data
            signalWaiter(waiter->guid, self->data, waiter->slot);
            waiters = waiter;
            waiter = waiter->next;
            free(waiters); // Release waiter node
        }
        if (hcSelf->kind == OCR_EVENT_ONCE_T) {
            // once-events die after satisfy has been called
            eventDestructor(base);
        }
    } else {
        // once-events cannot survive down here
        // idem-events ignore extra satisfy
        if (hcSelf->kind == OCR_EVENT_STICKY_T) {
            assert(false && "error: Multiple satisfy on a sticky event");
        }
    }
}

// Internal signal/wait notification
// Signal a sticky event some data arrived (on its unique slot)
static void singleEventSignaled(ocr_event_t * self, ocrGuid_t data, int slot) {
    // Single event signaled by another entity, call its satisfy method.
    singleEventSatisfy(self, data, slot);
}

ocrGuid_t singleEventGet (ocr_event_t * base) {
    hc_event_awaitable_t * self = (hc_event_awaitable_t*) base;
    return (self->data == UNINITIALIZED_DATA) ? ERROR_GUID : self->data;
}


//
// OCR-HC Latch Events Implementation
//


// Runs concurrently with registration on latch
void latchEventSatisfy(ocr_event_t * base, ocrGuid_t data, int slot) {
    assert((slot == OCR_EVENT_LATCH_DECR_SLOT) || (slot == OCR_EVENT_LATCH_INCR_SLOT));
    hc_event_latch_t * latch = (hc_event_latch_t *) base;
    int incr = (slot == OCR_EVENT_LATCH_DECR_SLOT) ? -1 : 1;
    int count;
    do {
        count = latch->counter;
    } while(!__sync_bool_compare_and_swap(&(latch->counter), count, count+incr));

    if ((count+incr) == 0) {
        hc_event_awaitable_t * self = (hc_event_awaitable_t *) base;
        //TODO add API to seal a list
        //TODO: do we need volatile here ?
        // CAS the wait-list
        volatile reg_node_t* waiters = self->waiters;
        // satisfy is concurrent with registration on the event
        // Try to cas the waiters list to SEALED_LIST to let others know the
        // latch has been satisfied
        while ( !__sync_bool_compare_and_swap( &(self->waiters), waiters, SEALED_LIST)) {
            // If we failed to cas, it means a new node has been inserted
            // in front of the registration list, so update our pointer and try again.
            waiters = self->waiters;
        }
        assert(self->waiters == SEALED_LIST);
        reg_node_t * waiter = (reg_node_t*) waiters;
        // Go over the list and notify waiters
        while (waiter != END_OF_LIST) {
            // For now latch events don't have any data output
            signalWaiter(waiter->guid, NULL_GUID, waiter->slot);
            waiters = waiter;
            waiter = waiter->next;
            free((reg_node_t*) waiters); // Release waiter node
        }
    }
}

// Internal signal/wait notification
// Signal a latch event some data arrived on one of its
static void latchEventSignaled(ocr_event_t * self, ocrGuid_t data, int slot) {
    // latch event signaled by another entity, call its satisfy method.
    latchEventSatisfy(self, data, slot);
}

// Latch-event doesn't have an output value
ocrGuid_t latchEventGet(ocr_event_t * base) {
    return NULL_GUID;
}


//
// OCR-HC Finish-Latch Events Implementation
//

#define FINISH_LATCH_DECR_SLOT 0
#define FINISH_LATCH_INCR_SLOT 1

// Requirements:
//  R1) All dependences (what the finish latch will satisfy) are provided at creation. This implementation DOES NOT support outstanding registrations.
//  R2) Number of incr and decr signaled on the event MUST BE equal.
void finishLatchEventSatisfy(ocr_event_t * base, ocrGuid_t data, int slot) {
    assert((slot == FINISH_LATCH_DECR_SLOT) || (slot == FINISH_LATCH_INCR_SLOT));
    hc_event_finishlatch_t * self = (hc_event_finishlatch_t *) base;
    int incr = (slot == FINISH_LATCH_DECR_SLOT) ? -1 : 1;
    int count;
    do {
        count = self->counter;
    } while(!__sync_bool_compare_and_swap(&(self->counter), count, count+incr));

    // No possible race when we reached 0 (see R2)
    if ((count+incr) == 0) {
        // Important to void the ELS at that point, to make sure there's no 
        // side effect on code executing downwards.
        ocrTask_t * task = getCurrentTask();
        task->els[ELS_SLOT_FINISH_LATCH] = NULL_GUID;
        // Notify waiters the latch is satisfied (We can extend that to a list // of waiters if we need to. (see R1))
        // Notify output event if any associated with the finish-edt
        reg_node_t * outputEventWaiter = &(self->outputEventWaiter);
        if (outputEventWaiter->guid != NULL_GUID) {
            signalWaiter(outputEventWaiter->guid, self->returnGuid, outputEventWaiter->slot);
        }
        // Notify the parent latch if any
        reg_node_t * parentLatchWaiter = &(self->parentLatchWaiter);
        if (parentLatchWaiter->guid != NULL_GUID) {
            ocr_event_t * parentLatch;
            globalGuidProvider->getVal(globalGuidProvider, parentLatchWaiter->guid, (u64*)&parentLatch, NULL);
            finishLatchCheckout(parentLatch);
        }
        // Since finish-latch is internal to finish-edt, and ELS is cleared, // there are no more pointers left to it, deallocate.
        eventDestructor(base);
    }
}

// Finish-latch event doesn't have an output value
ocrGuid_t finishLatchEventGet(ocr_event_t * base) {
    return NULL_GUID;
}


/******************************************************/
/* OCR-HC Finish-Latch Utilities                      */
/******************************************************/

// satisfies the incr slot of a finish latch event
static void finishLatchCheckin(ocr_event_t * base) {
    finishLatchEventSatisfy(base, NULL_GUID, FINISH_LATCH_INCR_SLOT);
}

// satisfies the decr slot of a finish latch event
static void finishLatchCheckout(ocr_event_t * base) {
    finishLatchEventSatisfy(base, NULL_GUID, FINISH_LATCH_DECR_SLOT);
}

static void setFinishLatch(ocrTask_t * edt, ocrGuid_t latchGuid) {
    edt->els[ELS_SLOT_FINISH_LATCH] = latchGuid;
}

static ocr_event_t * getFinishLatch(ocrTask_t * edt) {
    if (edt != NULL) { //  NULL happens in main when there's no edt yet
        ocrGuid_t latchGuid = edt->els[ELS_SLOT_FINISH_LATCH];
        if (latchGuid != NULL_GUID) {
            ocr_event_t * event = NULL;
            globalGuidProvider->getVal(globalGuidProvider, latchGuid, (u64*)&event, NULL);        
            return event;
        }
    }
    return NULL;
}

static bool isFinishLatchOwner(ocr_event_t * finishLatch, ocrGuid_t edtGuid) {
    return (finishLatch != NULL) && (((hc_event_finishlatch_t *)finishLatch)->ownerGuid == edtGuid);
}


/******************************************************/
/* OCR-HC Events Factory                              */
/******************************************************/

struct ocr_event_factory_struct* hc_event_factory_constructor(void) {
    hc_event_factory* derived = (hc_event_factory*) checked_malloc(derived, sizeof(hc_event_factory));
    ocr_event_factory* base = (ocr_event_factory*) derived;
    base->create = hcEventFactoryCreate;
    base->destruct =  hc_event_factory_destructor;
    // initialize singleton instance that carries hc implementation function pointers
    base->event_fct_ptrs_single = (ocr_event_fcts_t *) checked_malloc(base->event_fct_ptrs_single, sizeof(ocr_event_fcts_t));
    base->event_fct_ptrs_single->destruct = eventDestructor;
    base->event_fct_ptrs_single->get = singleEventGet;
    base->event_fct_ptrs_single->satisfy = singleEventSatisfy;

    // latch-events
    base->event_fct_ptrs_latch = (ocr_event_fcts_t *) checked_malloc(base->event_fct_ptrs_latch, sizeof(ocr_event_fcts_t));
    base->event_fct_ptrs_latch->destruct = eventDestructor;
    base->event_fct_ptrs_latch->get = latchEventGet;
    base->event_fct_ptrs_latch->satisfy = latchEventSatisfy;

    //Note: Just store finish-latch function ptrs in a static since this is 
    //      runtime implementation specific
    event_fct_ptrs_finishlatch = (ocr_event_fcts_t *) checked_malloc(event_fct_ptrs_finishlatch, sizeof(ocr_event_fcts_t));
    event_fct_ptrs_finishlatch->destruct = eventDestructor;
    event_fct_ptrs_finishlatch->get = finishLatchEventGet;
    event_fct_ptrs_finishlatch->satisfy = finishLatchEventSatisfy;

    return base;
}

void hc_event_factory_destructor ( struct ocr_event_factory_struct* base ) {
    hc_event_factory* derived = (hc_event_factory*) base;
    free(base->event_fct_ptrs_single);
    free(base->event_fct_ptrs_latch);
    free(event_fct_ptrs_finishlatch);
    free(derived);
}

// takesArg indicates whether or not this event carries any data
ocrGuid_t hcEventFactoryCreate ( struct ocr_event_factory_struct* factory, ocrEventTypes_t eventType, bool takesArg ) {
    ocr_event_t * res = eventConstructor(factory, eventType, takesArg);
    return res->guid;
}


/******************************************************/
/* OCR-HC Task Implementation                         */
/******************************************************/

// Forward declaration to keep related definitions together
static void taskSchedule( ocrGuid_t this_guid, ocr_task_t* base, ocrGuid_t wid );


void hcTaskConstructInternal (ocrTaskHc_t* derived, ocrEdt_t funcPtr,
        u32 paramc, u64 * params, void** paramv, size_t nbDeps, ocrGuid_t outputEvent, ocr_task_fcts_t * taskFctPtrs) {
    if (nbDeps == 0) {
        derived->signalers = END_OF_LIST;
    } else {
        // Since we know how many dependences we have, preallocate signalers
        derived->signalers = checked_malloc(derived->signalers, sizeof(reg_node_t)*nbDeps);
    }
    derived->waiters = END_OF_LIST;
    derived->nbdeps = nbDeps;
    derived->p_function = funcPtr;
    // Initialize base
    ocrTask_t* base = (ocrTask_t*) derived;
    base->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(base->guid), (u64)base, OCR_GUID_EDT);
    base->paramc = paramc;
    base->params = params;
    base->paramv = paramv;
    base->outputEvent = outputEvent;
    base->fct_ptrs = taskFctPtrs;
    // Initialize ELS
    int i = 0;
    while (i < ELS_SIZE) {
        base->els[i++] = NULL_GUID;
    }
}

ocrTaskHc_t* hcTaskConstruct (ocrEdt_t funcPtr, u32 paramc, u64 * params, void ** paramv, u16 properties, size_t depc, ocrGuid_t outputEvent, ocr_task_fcts_t * task_fct_ptrs) {

    ocrTaskHc_t* newEdt = (ocrTaskHc_t*)checked_malloc(newEdt, sizeof(ocrTaskHc_t));
    hcTaskConstructInternal(newEdt, funcPtr, paramc, params, paramv, depc, outputEvent, task_fct_ptrs);
    ocrTask_t * newEdtBase = (ocrTask_t *) newEdt;
    // If we are creating a finish-edt
    if (hasProperty(properties, EDT_PROP_FINISH)) {
        ocr_policy_domain_t* policy_domain = get_current_policy_domain();
        ocr_event_factory * eventFactory = policy_domain->getEventFactoryForUserEvents(policy_domain);
        ocr_event_t * latch = eventConstructor(eventFactory, OCR_EVENT_FINISH_LATCH_T, false);
        hc_event_finishlatch_t * hcLatch = (hc_event_finishlatch_t *) latch;
        // Set the owner of the latch 
        hcLatch->ownerGuid = newEdtBase->guid;
        ocr_event_t * parentLatch = getFinishLatch(getCurrentTask());
        if (parentLatch != NULL) {
            // Check in current finish latch
            finishLatchCheckin(parentLatch);
            // Link the new latch to the parent's decrement slot
            // When this finish scope is done, the parent scope is signaled.
            // DESIGN can modify the finish latch to have a standard list of regnode and just addDependence
            hcLatch->parentLatchWaiter.guid = parentLatch->guid;
            hcLatch->parentLatchWaiter.slot = FINISH_LATCH_DECR_SLOT;
        } else {
            hcLatch->parentLatchWaiter.guid = NULL_GUID;
            hcLatch->parentLatchWaiter.slot = -1;
        }
        // Check in the new finish scope
        finishLatchCheckin(latch);
        // Set edt's ELS to the new latch
        setFinishLatch(newEdtBase, latch->guid);
        // If there's an output event for this finish edt, add a dependence
        // from the finish latch to it.
        // DESIGN can modify flatch to have a standard list of regnode and just addDependence in a if !NULL_GUID
        hcLatch->outputEventWaiter.guid = outputEvent;
        hcLatch->outputEventWaiter.slot = 0;
    } else {
        // If the currently executing edt is in a finish scope, 
        // but is not a finish-edt itself, just register to the scope
        ocr_event_t * curLatch = getFinishLatch(getCurrentTask());
        if (curLatch != NULL) {
            // Check in current finish latch
            finishLatchCheckin(curLatch);
            // Transmit finish-latch to newly created edt
            setFinishLatch(newEdtBase, curLatch->guid);
        }
    }
    return newEdt;
}

void destructTaskHc ( ocrTask_t* base ) {
    ocrTaskHc_t* derived = (ocrTaskHc_t*)base;
    globalGuidProvider->releaseGuid(globalGuidProvider, base->guid);
    free(derived);
}

// Signals an edt one of its dependence slot is satisfied
void taskSignaled(ocrTask_t * base, ocrGuid_t data, int slot) {
    // An EDT has a list of signalers, but only register
    // incrementally as signals arrive.
    // Assumption: signal frontier is initialized at slot zero
    // Whenever we receive a signal, it can only be from the 
    // current signal frontier, since it is the only signaler
    // the edt is registered with at that time.
    ocrTaskHc_t * self = (ocrTaskHc_t *) base;
    // Replace the signaler's guid by the data guid, this to avoid
    // further references to the event's guid, which is good in general
    // and crucial for once-event since they are being destroyed on satisfy.
    self->signalers[slot].guid = data;
    if (slot == (self->nbdeps-1)) {
        // All dependencies have been satisfied, schedule the edt
        ocrGuid_t wid = ocr_get_current_worker_guid();
        taskSchedule(base->guid, base, wid);
    } else {
        // else register the edt on the next event to wait on
        slot++;
        //TODO, could we directly pass the node to avoid a new alloc ?
        registerWaiter((self->signalers[slot]).guid, base->guid, slot);
    }
}

//Registers an entity that will signal on one of the edt's slot.
static void edtRegisterSignaler(ocrTask_t * base, ocrGuid_t signalerGuid, int slot) {
    // Only support event signals
    assert(isEventGuid(signalerGuid) || isDatablockGuid(signalerGuid));
    //DESIGN would be nice to pre-allocate all of this since we know the edt
    //       dep slot size at edt's creation, then what type should we expose
    //       for the signalers member in the task's data-structure ?
    //No need to CAS anything as all dependencies are provided at creation.
    ocrTaskHc_t * self = (ocrTaskHc_t *) base;
    reg_node_t * node = &(self->signalers[slot]);
    node->guid = signalerGuid;
    node->slot = slot;
    // No need to chain nodes here, will use index
    node->next = NULL; 
}


//
// OCR TASK SCHEDULING
//

/**
 * @brief Schedules a task when we know its guid.
 * Warning: The caller must ensure all dependencies have been satisfied
 * Note: static function only meant to factorize code.
 */
void taskSchedule( ocrGuid_t guid, ocrTask_t* base, ocrGuid_t wid ) {
    ocr_worker_t* w = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wid, (u64*)&w, NULL);
    ocr_scheduler_t * scheduler = get_worker_scheduler(w);
    scheduler->give(scheduler, wid, guid);
}

/**
 * @brief Tries to schedules a task by registering to its first dependence.
 * If no dependence, schedule the task right-away 
 * Warning: This method is to be called ONCE per task and there's no safeguard !
 */
void tryScheduleTask( ocrTask_t* base, ocrGuid_t wid ) {
    //DESIGN This is very much specific to the way we've chosen
    //       to handle event-to-task dependence, which needs some
    //       kind of bootstrapping. This could be done when the last
    //       dependence slot is registered with a signaler, but then
    //       the semantic of the schedule function is weaker.
    ocrTaskHc_t* self = (ocrTaskHc_t*)base;
    if (self->nbdeps != 0) {
        // Register the task on the first dependence. If all
        // dependences are here, the task is scheduled by
        // the dependence management code.
        registerWaiter(self->signalers[0].guid, base->guid, 0);
    } else {
        // If there's no dependence, the task must be scheduled now.
        taskSchedule(base->guid, base, wid);
    }
}

void taskExecute ( ocrTask_t* base ) {
    ocrTaskHc_t* derived = (ocrTaskHc_t*)base;
    // In this implementation each time a signaler has been satisfied, its guid
    // has been replaced by the db guid it has been satisfied with.
    int nbdeps = derived->nbdeps;
    ocrEdtDep_t * depv = NULL;
    // If any dependencies, acquire their data-blocks
    if (nbdeps != 0) {
        size_t i = 0;
        //TODO would be nice to resolve regNode into guids before
        depv = (ocrEdtDep_t *) checked_malloc(depv, sizeof(ocrEdtDep_t) * nbdeps);
        // Double-check we're not rescheduling an already executed edt
        assert(derived->signalers != END_OF_LIST);
        while ( i < nbdeps ) {
            //TODO would be nice to standardize that on satisfy
            reg_node_t * regNode = &(derived->signalers[i]);
            ocrGuid_t dbGuid = regNode->guid;
            depv[i].guid = dbGuid;
            if(dbGuid != NULL_GUID) {
                assert(isDatablockGuid(dbGuid));
                ocrDataBlock_t * db = NULL;
                globalGuidProvider->getVal(globalGuidProvider, dbGuid, (u64*)&db, NULL);
                depv[i].ptr = db->acquire(db, base->guid, true);
            } else {
                depv[i].ptr = NULL;
            }
            i++;
        }
        free(derived->signalers);
        derived->signalers = END_OF_LIST;
    }

    ocrGuid_t retGuid = derived->p_function(base->paramc, base->params, base->paramv, nbdeps, depv);

    // edt user code is done, if any deps, release data-blocks
    if (nbdeps != 0) {
        size_t i = 0;
        for(i=0; i<nbdeps; ++i) {
            if(depv[i].guid != NULL_GUID) {
                ocrDataBlock_t * db = NULL;
                globalGuidProvider->getVal(globalGuidProvider, depv[i].guid, (u64*)&db, NULL);
                RESULT_ASSERT(db->release(db, base->guid, true), ==, 0);
            }
        }
    }

    // check out from current finish scope
    ocr_event_t * curLatch = getFinishLatch(base);

    if (curLatch != NULL) {
        // if own the latch, then set the retGuid
        if (isFinishLatchOwner(curLatch, base->guid)) {
            ((hc_event_finishlatch_t *) curLatch)->returnGuid = retGuid;
        }
        finishLatchCheckout(curLatch);
        // If the edt is the last to checkout from the current finish scope,
        // the latch event automatically satisfies the parent latch (if any) // and the output event associated with the current finish-edt (if any)
    } 
    // When the edt is not a finish-edt, nobody signals its output
    // event but himself, do that now.
    if ((base->outputEvent != NULL_GUID) && !isFinishLatchOwner(curLatch, base->guid)) {
        ocr_event_t * outputEvent;
        globalGuidProvider->getVal(globalGuidProvider, base->outputEvent, (u64*)&outputEvent, NULL);
        // We know the output event must be of type single sticky since it is
        // internally allocated by the runtime
        assert(isEventSingleGuid(base->outputEvent));
        singleEventSatisfy(outputEvent, retGuid, 0);
    }
}

/******************************************************/
/* OCR-HC Task Factory                                */
/******************************************************/

ocrGuid_t newTaskHc(ocrTaskFactory_t* factory, ocrEdt_t fctPtr, u32 paramc, u64 * params, void** paramv, u16 properties, size_t depc, ocrGuid_t * outputEventPtr) {
    // Initialize a sticky outputEvent if requested
    ocrGuid_t outputEvent = (ocrGuid_t) outputEventPtr;
    if (outputEvent != NULL_GUID) {
        outputEvent = hcEventFactoryCreate(eventFactory, OCR_EVENT_STICKY_T, false);    
        *outputEventPtr = outputEvent;
    }
    ocrTaskHc_t* edt = hcTaskConstruct(fctPtr, paramc, params, paramv, properties, depc, outputEvent, factory->task_fct_ptrs);
    ocrTask_t* base = (ocrTask_t*) edt;
    return base->guid;
}

void destructTaskFactoryHc(ocrTaskFactory_t* base) {
    ocrTaskFactoryHc_t* derived = (ocrTaskFactoryHc_t*) base;
    free(base->task_fct_ptrs);
    free(derived);
}

ocrTaskFactory_t * newTaskFactoryHc(void * config) {
    ocrTaskFactoryHc_t* derived = (ocrTaskFactoryHc_t*) checked_malloc(derived, sizeof(ocrTaskFactoryHc_t));
    ocrTaskFactory_t* base = (ocrTaskFactory_t*) derived;
    base->instantiate = newTaskHc;
    base->destruct =  destructTaskFactoryHc;
    // initialize singleton instance that carries hc implementation 
    // function pointers. Every instantiated task will use this pointer
    // to resolve functions implementations.
    base->task_fct_ptrs = (ocr_task_fcts_t *) checked_malloc(base->task_fct_ptrs, sizeof(ocr_task_fcts_t));
    base->task_fct_ptrs->destruct = destructTaskHc;
    base->task_fct_ptrs->execute = taskExecute;
    base->task_fct_ptrs->schedule = tryScheduleTask;
    return base;
}


/******************************************************/
/* Signal/Wait interface implementation               */
/******************************************************/

//These are essentially switches to dispatch call to the correct implementation

void registerDependence(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot) {
    // WAIT MODE: event-to-event registration
    // Note: do not call 'registerWaiter' here as it triggers event-to-edt
    // registration, which should only be done on edtSchedule.
    if (isEventGuid(signalerGuid) && isEventGuid(waiterGuid)) {
        hc_event_awaitable_t * target;
        globalGuidProvider->getVal(globalGuidProvider, signalerGuid, (u64*)&target, NULL);
        awaitableEventRegisterWaiter(target, waiterGuid, slot);
        return;
    }

    // SIGNAL MODE:
    //  - anything-to-edt registration
    //  - db-to-event registration
    registerSignaler(signalerGuid, waiterGuid, slot);
}

// Registers a waiter on a signaler
static void registerWaiter(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot) {
    if (isEventGuid(signalerGuid)) {
        assert(isEdtGuid(waiterGuid) || isEventGuid(waiterGuid));
        hc_event_awaitable_t * target;
        globalGuidProvider->getVal(globalGuidProvider, signalerGuid, (u64*)&target, NULL);
        awaitableEventRegisterWaiter(target, waiterGuid, slot);
    } else if(isDatablockGuid(signalerGuid) && isEdtGuid(waiterGuid)) {
            signalWaiter(waiterGuid, signalerGuid, slot);
    } else {
        // Everything else is an error
        assert("error: Unsupported guid kind in registerWaiter" );
    }
}

// register a signaler on a waiter
static void registerSignaler(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot) {
    // anything to edt registration
    if (isEdtGuid(waiterGuid)) {
        // edt waiting for a signal from an event or a datablock
        assert(isEventGuid(signalerGuid) || isDatablockGuid(signalerGuid));
        ocrTask_t * target = NULL;
        globalGuidProvider->getVal(globalGuidProvider, waiterGuid, (u64*)&target, NULL);
        edtRegisterSignaler(target, signalerGuid, slot);
        return;
    // datablock to event registration => satisfy on the spot
    } else if (isDatablockGuid(signalerGuid)) {
        assert(isEventGuid(waiterGuid));
        ocr_event_t * target = NULL;
        globalGuidProvider->getVal(globalGuidProvider, waiterGuid, (u64*)&target, NULL);
        // This looks a duplicate of signalWaiter, however there hasn't
        // been any signal strictly speaking, hence calling satisfy directly
        if (isEventSingleGuid(waiterGuid)) {
            singleEventSatisfy(target, signalerGuid, slot);
        } else {
            assert(isEventLatchGuid(waiterGuid));
            latchEventSatisfy(target, signalerGuid, slot);
        }
        return;
    }
    // Remaining legal registrations is event-to-event, but this is
    // handled in 'registerWaiter'
    assert(isEventGuid(waiterGuid) && isEventGuid(signalerGuid) && "error: Unsupported guid kind in registerDependence");
}

// signal a 'waiter' data has arrived on a particular slot
static void signalWaiter(ocrGuid_t waiterGuid, ocrGuid_t data, int slot) {
    // TODO do we need to know who's signaling ?
    if (isEventSingleGuid(waiterGuid)) {
        ocr_event_t * target = NULL;
        globalGuidProvider->getVal(globalGuidProvider, waiterGuid, (u64*)&target, NULL);
        singleEventSignaled(target, data, slot);
    } else if (isEventLatchGuid(waiterGuid)) {
        ocr_event_t * target = NULL;
        globalGuidProvider->getVal(globalGuidProvider, waiterGuid, (u64*)&target, NULL);
        latchEventSignaled(target, data, slot);
    } else if(isEdtGuid(waiterGuid)) {
        ocrTask_t * target = NULL;
        globalGuidProvider->getVal(globalGuidProvider, waiterGuid, (u64*)&target, NULL);
        taskSignaled(target, data, slot);
    } else {
        // ERROR
        assert(0 && "error: Unsupported guid kind in signal");
    }
}
