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
#include "debug.h"

#define SEALED_LIST ((void *) -1)
#define END_OF_LIST NULL
#define UNINITIALIZED_DATA ((ocrGuid_t) -2)

//
// Guid-kind checkers for convenience
//

static bool isEventGuid(ocrGuid_t guid) {
    ocrGuidKind kind;
    globalGuidProvider->getKind(globalGuidProvider, guid, &kind);
    printf("isEventGuid %p, kind is %d\n", (void *)guid, kind);
    return kind == OCR_GUID_EVENT;
}

static bool isEdtGuid(ocrGuid_t guid) {
    ocrGuidKind kind;
    globalGuidProvider->getKind(globalGuidProvider, guid, &kind);
    printf("isEdtGuid %p, kind is %d\n", (void *)guid, kind);
    return kind == OCR_GUID_EDT;
}

static bool isEventStickyGuid(ocrGuid_t guid) {
    if(isEventGuid(guid)) {
        hc_event_t * event = NULL;
        globalGuidProvider->getVal(globalGuidProvider, guid, (u64*)&event, NULL);
        printf("isEventStickyGuid %p, kind is %d\n", (void *)guid, event->kind);   
        return event->kind == OCR_EVENT_STICKY_T;
    }
    return false;
}


// forward declarations
static void signalWaiter(ocrGuid_t waiterGuid, ocrGuid_t data, int slot);

static void registerSignaler(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot, bool offer);

static void registerWaiter(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot, bool offer);

/******************************************************/
/* OCR-HC Events Implementation                       */
/******************************************************/

//
// OCR-HC Events Life-Cycle
//

// Generic initializer for events. Implementation-dependent event functions
// are passed through the function pointer data-structure.
struct ocr_event_struct* eventConstructor(ocrEventTypes_t eventType, bool takesArg, ocr_event_fcts_t * eventFctPtrs) {
    hc_event_t* derived = (hc_event_t*) checked_malloc(derived, sizeof(hc_event_t));
    derived->kind = eventType;
    derived->signalers = END_OF_LIST;
    derived->waiters = END_OF_LIST;
    derived->data = UNINITIALIZED_DATA;
    ocr_event_t* base = (ocr_event_t*)derived;
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
// OCR-HC Sticky Events Implementation
//

// Sticky event registration registers a 'waiter' that should be
// signaled by the 'self' event on a particular slot.
// If 'self' has already been satisfied, signal 'waiter'
// Warning: Concurrent with sticky event's put.
static void stickyEventRegisterWaiter(hc_event_t * self, ocrGuid_t waiter, int slot) {
    // Try to insert 'waiter' at the beginning of the list
    reg_node_t * curHead = self->waiters;
    if(curHead != SEALED_LIST) {
        reg_node_t * newHead = checked_malloc(newHead, sizeof(reg_node_t));
        newHead->guid = waiter;
        newHead->slot = slot;
        newHead->next = curHead;
        while(curHead != SEALED_LIST && !__sync_bool_compare_and_swap(&(self->waiters), curHead, newHead)) {
            curHead = self->waiters;
            newHead->next = curHead;
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

// Set a sticky event data guid
// Warning: Concurrent with registration on sticky event.
static reg_node_t * stickyEventPut ( hc_event_t* self, ocrGuid_t data ) {
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
void stickyEventSatisfy(ocr_event_t * base, ocrGuid_t data, int slotEvent) {
    hc_event_t * self = (hc_event_t *) base;
    // Sticky events don't have slots, just put the data
    reg_node_t * waiters = stickyEventPut(self, data);
    // Put must have sealed the waiters list and returned it
    // Note: Here the design is not great, 'put' CAS and returns the waiters 
    // list but does not handle the signaler list which is ok for now because 
    // it's not used.
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
}

// Internal signal/wait notification
// Signal a sticky event some data arrived (on its unique slot)
void stickyEventSignaled(ocr_event_t * self, ocrGuid_t data, int slotEvent) {
    // Sticky event signaled by another entity, call its satisfy method.
    stickyEventSatisfy(self, data, slotEvent);
}

ocrGuid_t stickyEventGet (ocr_event_t * base) {
    hc_event_t* self = (hc_event_t*) base;
    return (self->data == UNINITIALIZED_DATA) ? ERROR_GUID : self->data;
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
    base->event_fct_ptrs_sticky = (ocr_event_fcts_t *) checked_malloc(base->event_fct_ptrs_sticky, sizeof(ocr_event_fcts_t));
    base->event_fct_ptrs_sticky->destruct = eventDestructor;
    base->event_fct_ptrs_sticky->get = stickyEventGet;
    base->event_fct_ptrs_sticky->satisfy = stickyEventSatisfy;
    return base;
}

void hc_event_factory_destructor ( struct ocr_event_factory_struct* base ) {
    hc_event_factory* derived = (hc_event_factory*) base;
    free(base->event_fct_ptrs_sticky);
    free(derived);
}

// takesArg indicates whether or not this event carries any data
ocrGuid_t hcEventFactoryCreate ( struct ocr_event_factory_struct* factory, ocrEventTypes_t eventType, bool takesArg ) {
    ocr_event_fcts_t * eventFctPtrs = NULL;
    if (eventType == OCR_EVENT_STICKY_T) {
        eventFctPtrs = factory->event_fct_ptrs_sticky;
    } else {
        assert("limitation: Only sticky events are supported" && false);
    }
    ocr_event_t * res = eventConstructor(eventType, takesArg, eventFctPtrs);
    return res->guid;
}

/******************************************************/
/* OCR-HC Task Implementation                         */
/******************************************************/

// Forward declaration to keep related definitions together
static void taskSchedule( ocrGuid_t this_guid, ocr_task_t* base, ocrGuid_t wid );

static void hcTaskConstructInternal (hc_task_t* derived, ocrEdt_t funcPtr,
        u32 paramc, u64 * params, void** paramv, size_t nbDeps, ocr_task_fcts_t * taskFctPtrs) {
    if (nbDeps == 0) {
        derived->signalers = END_OF_LIST;
    } else {
        // Since we know how many dependences we have, preallocate signalers
        derived->signalers = checked_malloc(derived->signalers, sizeof(reg_node_t)*nbDeps);
    }
    derived->waiters = END_OF_LIST;
    derived->nbdeps = nbDeps;
    derived->depv = NULL;
    derived->p_function = funcPtr;
    ocr_task_t* base = (ocr_task_t*) derived;
    base->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(base->guid), (u64)base, OCR_GUID_EDT);
    base->paramc = paramc;
    base->params = params;
    base->paramv = paramv;
    base->fct_ptrs = taskFctPtrs;
}

hc_task_t* hcTaskConstruct (ocrEdt_t funcPtr, u32 paramc, u64 * params, void** paramv, size_t dep_list_size, ocr_task_fcts_t * task_fct_ptrs) {
    hc_task_t* derived = (hc_task_t*)checked_malloc(derived, sizeof(hc_task_t));
    hcTaskConstructInternal(derived, funcPtr, paramc, params, paramv, dep_list_size, task_fct_ptrs);
    return derived;
}

void hcTaskDestruct ( ocr_task_t* base ) {
    hc_task_t* derived = (hc_task_t*)base;
    globalGuidProvider->releaseGuid(globalGuidProvider, base->guid);
    free(derived);
}

// Signals an edt one of its dependence slot is satisfied 
void taskSignaled(ocr_task_t * base, ocrGuid_t data, int slot) {
    // An EDT has a list of signalers, but only register
    // incrementally as signals arrive.
    // Assumption: signal frontier is initialized at slot zero
    // Whenever we receive a signal, it can only be from the 
    // current signal frontier, since it is the only signaler
    // the edt is registered with at that time.
    hc_task_t * self = (hc_task_t *) base;
    if (slot == (self->nbdeps-1)) {
        // All dependencies have been satisfied, schedule the edt
        ocrGuid_t wid = ocr_get_current_worker_guid();
        taskSchedule(base->guid, base, wid);
    } else {
        // else register the edt on the next event to wait on
        slot++;
        //TODO, could we directly pass the node to avoid a new alloc ?
        registerWaiter((self->signalers[slot]).guid, base->guid, slot, false);
    }
}

//Registers an entity that will signal on one of the edt's slot.
static void edtRegisterSignaler(ocr_task_t * base, ocrGuid_t signalerGuid, int slot) {
    // Only support event signals
    assert(isEventGuid(signalerGuid));
    //DESIGN would be nice to pre-allocate all of this since we know the edt
    //       dep slot size at edt's creation, then what type should we expose
    //       for the signalers member in the task's data-structure ?
    //No need to CAS anything as all dependencies are provided at creation.
    hc_task_t * self = (hc_task_t *) base;
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
static void taskSchedule( ocrGuid_t guid, ocr_task_t* base, ocrGuid_t wid ) {
    printf("schedule task\n");
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
void tryScheduleTask( ocr_task_t* base, ocrGuid_t wid ) {
    //DESIGN This is very much specific to the way we've chosen
    //       to handle event-to-task dependence, which needs some
    //       kind of bootstrapping. This could be done when the last
    //       dependence slot is registered with a signaler, but then
    //       the semantic of the schedule function is weaker.
    hc_task_t* self = (hc_task_t*)base;
    if (self->nbdeps != 0) {
        // Register the task on the first dependence. If all
        // dependences are here, the task is scheduled by
        // the dependence management code.
        registerWaiter(self->signalers[0].guid, base->guid, 0, false);
    } else {
        // If there's no dependence, the task must be scheduled now.
        taskSchedule(base->guid, base, wid);
    }
}

void taskExecute ( ocr_task_t* base ) {
    hc_task_t* derived = (hc_task_t*)base;
    // In this implementation the list of signalers is a list of events
    ocr_event_t * eventDep = NULL;
    ocrDataBlock_t *db = NULL;
    ocrGuid_t dbGuid = UNINITIALIZED_GUID;
    size_t i = 0;
    int nbdeps = derived->nbdeps;
    ocrEdtDep_t * depv = NULL;
    printf("Task execute called \n");
    // If any dependencies, acquire their data-blocks
    if (nbdeps != 0) {
        //TODO would be nice to resolve regNode into event_t before
        depv = (ocrEdtDep_t *) checked_malloc(depv, sizeof(ocrEdtDep_t) * nbdeps);
        derived->depv = depv;
        // Double-check we're not rescheduling an already executed edt
        assert(derived->signalers != END_OF_LIST);
        while ( i < nbdeps ) {
            reg_node_t * regNode = &(derived->signalers[i]);
            globalGuidProvider->getVal(globalGuidProvider, regNode->guid, (u64*)&eventDep, NULL);
            dbGuid = eventDep->fct_ptrs->get(eventDep);
            depv[i].guid = dbGuid;
            if(dbGuid != NULL_GUID) {
                db = NULL;
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

    derived->p_function(base->paramc, base->params, base->paramv, nbdeps, depv);

    // edt user code is done, if any deps, release data-blocks
    if (nbdeps != 0) {
        for(i=0; i<nbdeps; ++i) {
            if(depv[i].guid != NULL_GUID) {
                globalGuidProvider->getVal(globalGuidProvider, depv[i].guid, (u64*)&db, NULL);
                RESULT_ASSERT(db->release(db, base->guid, true), ==, 0);
            }
        }
        free(depv);
    }
}


/******************************************************/
/* OCR-HC Task Factory                                */
/******************************************************/

struct ocr_task_factory_struct* hc_task_factory_constructor(void) {
    hc_task_factory* derived = (hc_task_factory*) checked_malloc(derived, sizeof(hc_task_factory));
    ocr_task_factory* base = (ocr_task_factory*) derived;
    base->create = hc_task_factory_create;
    base->destruct =  hc_task_factory_destructor;

    // initialize singleton instance that carries hc implementation function pointers
    base->task_fct_ptrs = (ocr_task_fcts_t *) checked_malloc(base->task_fct_ptrs, sizeof(ocr_task_fcts_t));
    base->task_fct_ptrs->destruct = hcTaskDestruct;
    base->task_fct_ptrs->execute = taskExecute;
    base->task_fct_ptrs->schedule = tryScheduleTask;
    return base;
}

void hc_task_factory_destructor ( struct ocr_task_factory_struct* base ) {
    hc_task_factory* derived = (hc_task_factory*) base;
    free(base->task_fct_ptrs);
    free(derived);
}

ocrGuid_t hc_task_factory_create ( struct ocr_task_factory_struct* factory, ocrEdt_t fctPtr, u32 paramc, u64 * params, void** paramv, size_t dep_l_size) {
    hc_task_t* edt = hcTaskConstruct(fctPtr, paramc, params, paramv, dep_l_size, factory->task_fct_ptrs);
    ocr_task_t* base = (ocr_task_t*) edt;
    return base->guid;
}

/******************************************************/
/* Signal/Wait interface implementation               */
/******************************************************/

//These are essentially switches to dispatch call to the correct implementation

void offerWaiterRegistration(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot) {
    registerWaiter(signalerGuid, waiterGuid, slot, true);
}

void offerSignalerRegistration(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot) {
    registerSignaler(signalerGuid, waiterGuid, slot, true);
}

// Registers a waiter on a signaler
static void registerWaiter(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot, bool offer) {
    if (isEventStickyGuid(signalerGuid)) {
            if (offer && isEdtGuid(waiterGuid)) {
                // When we're in offering mode, the implementation doesn't
                // register edts on events they depend on. This will be done
                // on edtSchedule.
                return;
            }
            hc_event_t * target;
            globalGuidProvider->getVal(globalGuidProvider, signalerGuid, (u64*)&target, NULL);
            stickyEventRegisterWaiter(target, waiterGuid, slot);
    } else if(isEdtGuid(signalerGuid)) {
        // register a 'waiter' to be signaled when an edt is done
        assert(0 && "limitation: register edt-to-entity waiter not supported");
    } else {
        // ERROR
        assert(0 && "error: Unsupported guid kind in registerWaiter");
    }
}

// register a signaler on a waiter
static void registerSignaler(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot,bool offer) {
    printf("Checking registerSignaler %p, %p\n", (void *) signalerGuid, (void*) waiterGuid);
    if (isEdtGuid(waiterGuid)) {
        if (isEventGuid(signalerGuid)) {
            // edt waiting for a signal from an event
            ocr_task_t * target = NULL;
            globalGuidProvider->getVal(globalGuidProvider, waiterGuid, (u64*)&target, NULL);
            edtRegisterSignaler(target, signalerGuid, slot);
        } else if(isEdtGuid(signalerGuid)) {
           assert(0 && "limitation: register edt-to-edt signal not supported");
        } else {
           assert(0 && "error: Unsupported guid kind in registerSignaler");
        }
    } else if(isEventGuid(waiterGuid)) {
          if (isEventGuid(signalerGuid)) {

          } else {
           assert(0 && "error: Unsupported guid kind in registerSignaler"); 
          }
        // We do not register events as signalers on waiters
    }
    printf("END Checking registerSignaler \n");
}

// signal a 'waiter' data has arrived on a particular slot
static void signalWaiter(ocrGuid_t waiterGuid, ocrGuid_t data, int slot) {
    // TODO do we need to know who's signaling ?
    if (isEventStickyGuid(waiterGuid)) {
        ocr_event_t * target = NULL;
        globalGuidProvider->getVal(globalGuidProvider, waiterGuid, (u64*)&target, NULL);
        stickyEventSignaled(target, data, slot);
    } else if(isEdtGuid(waiterGuid)) {
        ocr_task_t * target = NULL;
        globalGuidProvider->getVal(globalGuidProvider, waiterGuid, (u64*)&target, NULL);
        taskSignaled(target, data, slot);
    } else {
        // ERROR
        assert(0 && "error: Unsupported guid kind in signal");
    }
}

