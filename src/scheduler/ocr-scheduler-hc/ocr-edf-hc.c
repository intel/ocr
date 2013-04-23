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

struct ocr_event_factory_struct* hc_event_factory_constructor(void) {
    hc_event_factory* derived = (hc_event_factory*) checked_malloc(derived, sizeof(hc_event_factory));
    ocr_event_factory* base = (ocr_event_factory*) derived;
    base->create = hc_event_factory_create;
    base->destruct =  hc_event_factory_destructor;
    // initialize singleton instance that carries hc implementation function pointers
    base->event_fct_ptrs_sticky = (ocr_event_fcts_t *) checked_malloc(base->event_fct_ptrs_sticky, sizeof(ocr_event_fcts_t));
    base->event_fct_ptrs_sticky->destruct = hc_event_destructor;
    base->event_fct_ptrs_sticky->get = hc_event_get;
    base->event_fct_ptrs_sticky->put = hc_event_put;
    base->event_fct_ptrs_sticky->registerIfNotReady = hc_event_register_if_not_ready;
    return base;
}

void hc_event_factory_destructor ( struct ocr_event_factory_struct* base ) {
    hc_event_factory* derived = (hc_event_factory*) base;
    free(base->event_fct_ptrs_sticky);
    free(derived);
}

ocrGuid_t hc_event_factory_create ( struct ocr_event_factory_struct* factory, ocrEventTypes_t eventType, bool takesArg ) {
    //TODO LIMITATION Support other events types
    if (eventType != OCR_EVENT_STICKY_T) {
        assert("LIMITATION: Only sticky events are supported" && false);
    }
    // takesArg indicates whether or not this event carries any data

    // If not one can provide a more compact implementation
    //This would have to be different for different types of events
    //We can have a switch here and dispatch call to different event constructors
    ocr_event_t * res = hc_event_constructor(eventType, takesArg, factory->event_fct_ptrs_sticky);
    return res->guid;
}

#define UNINITIALIZED_REGISTER_LIST ((register_list_node_t*) -1)
#define EMPTY_REGISTER_LIST (NULL)

struct ocr_event_struct* hc_event_constructor(ocrEventTypes_t eventType, bool takesArg, ocr_event_fcts_t * event_fct_ptrs_sticky) {
    hc_event_t* derived = (hc_event_t*) checked_malloc(derived, sizeof(hc_event_t));
    derived->datum = UNINITIALIZED_GUID;
    derived->register_list = UNINITIALIZED_REGISTER_LIST;
    ocr_event_t* base = (ocr_event_t*)derived;

    base->fct_ptrs = event_fct_ptrs_sticky;

    base->guid = UNINITIALIZED_GUID;

    globalGuidProvider->getGuid(globalGuidProvider, &(base->guid), (u64)base, OCR_GUID_EVENT);
    /*
    base->destruct = hc_event_destructor;
    base->get = hc_event_get;
    base->put = hc_event_put;
    base->registerIfNotReady = hc_event_register_if_not_ready;
    */
    return base;
}

void hc_event_destructor ( struct ocr_event_struct* base ) {
    hc_event_t* derived = (hc_event_t*)base;
    globalGuidProvider->releaseGuid(globalGuidProvider, base->guid);
    free(derived);
}

ocrGuid_t hc_event_get (struct ocr_event_struct* event) {
    hc_event_t* derived = (hc_event_t*)event;
    if ( derived->datum == UNINITIALIZED_GUID ) return ERROR_GUID;
    return derived->datum;
}

register_list_node_t* hc_event_compete_for_put ( hc_event_t* derived, ocrGuid_t data_for_put_id ) {
    // TODO don't think this is enough to detect concurrent puts.
    assert ( derived->datum == UNINITIALIZED_GUID && "violated single assignment property for EDFs");

    volatile register_list_node_t* registerListOfEDF = NULL;

    derived->datum = data_for_put_id;
    registerListOfEDF = derived->register_list;
    // 'put' may be concurrent to an another edt trying to register a task to this event.
    // Try to cas the registration list to EMPTY_REGISTER_LIST to let other know the 'put' has occurred.
    while ( !__sync_bool_compare_and_swap( &(derived->register_list), registerListOfEDF, EMPTY_REGISTER_LIST)) {
        // If we failed to cas, it means a new node has been inserted
        // in front of the registration list, so update our pointer and try again.
        registerListOfEDF = derived->register_list;
    }
    // return the most up to date head on the registration list
    // (no more adds possible once EMPTY_REGISTER_LIST has been set)
    return (register_list_node_t*) registerListOfEDF;
}


// Forward declaration to keep related definitions together
static void hc_task_schedule( ocrGuid_t this_guid, ocr_task_t* base, ocrGuid_t wid );

/**
 * @brief Signal edts waiting on an event a put has occurred
 */
void hc_event_signal_waiters( register_list_node_t* task_id_list ) {
    register_list_node_t* curr = task_id_list;
    // Only try to notify if someone is waiting on the event
    if (UNINITIALIZED_REGISTER_LIST != curr) {
        // Lookup current worker id outside of the
        // loop to avoid multiple pthread lookups
        register_list_node_t* next = NULL;
        ocrGuid_t wid = ocr_get_current_worker_guid();
        while ( UNINITIALIZED_REGISTER_LIST != curr ) {
            ocrGuid_t curr_task_guid = curr->task_guid;
            ocr_task_t* curr_task = NULL;
            globalGuidProvider->getVal(globalGuidProvider, curr_task_guid, (u64*)&curr_task, NULL);

            // Try to iterate the frontier of the task and schedule it if successful
            if ( curr_task->fct_ptrs->iterate_waiting_frontier(curr_task) ) {
                hc_task_schedule(curr_task_guid, curr_task, wid);
            }
            next = curr->next;
            free(curr); // free the registration node
            curr = next;
        }
    }
}

void hc_event_put (struct ocr_event_struct* event, ocrGuid_t db) {
    hc_event_t* derived = (hc_event_t*)event;
    register_list_node_t* task_list = hc_event_compete_for_put(derived, db);
    // At this point, task_list cannot be modified anymore
    hc_event_signal_waiters(task_list);
}

/**
 * @brief Add a task to an event registration list.
 * @return true if the event has been registered. false indicates the event has been satisfied
 */
bool hc_event_register_if_not_ready(struct ocr_event_struct* event, ocrGuid_t polling_task_id ) {
    hc_event_t* derived = (hc_event_t*)event;
    bool success = false;
    volatile register_list_node_t* registerListOfEDF = derived -> register_list;

    if ( registerListOfEDF != EMPTY_REGISTER_LIST ) {
        register_list_node_t* new_node = (register_list_node_t*) checked_malloc(new_node, sizeof(register_list_node_t));
        new_node->task_guid = polling_task_id;

        // Try to add the polling task registration in front of the event registration list.
        while ( registerListOfEDF != EMPTY_REGISTER_LIST && !success ) {
            new_node -> next = (register_list_node_t*) registerListOfEDF;

            // compete with 'put' and other tasks trying to register on the event
            success = __sync_bool_compare_and_swap(&(derived -> register_list), registerListOfEDF, new_node);

            if ( !success ) {
                registerListOfEDF = derived -> register_list;
            }
        }
        if (!success) {
            // The task became ready while we were trying to register.
            // Since we couldn't, discard the newly created registration node.
            free(new_node);
        }
    }
    return success;
}

hc_await_list_t* hc_await_list_constructor( size_t al_size ) {

    hc_await_list_t* derived = (hc_await_list_t*) checked_malloc(derived, sizeof(hc_await_list_t));
    derived->array = checked_malloc(derived->array, sizeof(ocr_event_t*) * (al_size+1));
    derived->array[al_size] = NULL;
    derived->waitingFrontier = &derived->array[0];
    return derived;
}

hc_await_list_t* hc_await_list_constructor_with_event_list ( event_list_t* el) {
    hc_await_list_t* derived = (hc_await_list_t*) checked_malloc(derived, sizeof(hc_await_list_t));
    derived->array = checked_malloc(derived->array, sizeof(ocr_event_t*)*(el->size+1));
    derived->waitingFrontier = &derived->array[0];
    size_t i, size = el->size;
    event_list_node_t* curr = el->head;
    for ( i = 0; i < size; ++i, curr = curr -> next ) {
        derived->array[i] = curr->event;
    }
    derived->array[el->size] = NULL;
    return derived;
}

void hc_await_list_destructor( hc_await_list_t* derived ) {
    free(derived->array);
    free(derived);
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
    base->task_fct_ptrs->destruct = hc_task_destruct;
    base->task_fct_ptrs->iterate_waiting_frontier = hc_task_iterate_waiting_frontier;
    base->task_fct_ptrs->execute = hc_task_execute;
    base->task_fct_ptrs->schedule = hc_task_try_schedule;
    base->task_fct_ptrs->add_dependence = hc_task_add_dependence;
    return base;
}

void hc_task_factory_destructor ( struct ocr_task_factory_struct* base ) {
    hc_task_factory* derived = (hc_task_factory*) base;
    free(base->task_fct_ptrs);
    free(derived);
}

ocrGuid_t hc_task_factory_create_with_event_list (struct ocr_task_factory_struct* factory, ocrEdt_t fctPtr, u32 paramc, u64 * params, void** paramv, ocrGuid_t outputEvent, event_list_t* l) {
    hc_task_t* edt = hc_task_construct_with_event_list(fctPtr, paramc, params, paramv, l, outputEvent, factory->task_fct_ptrs);
    ocr_task_t* base = (ocr_task_t*) edt;
    return base->guid;
}

ocrGuid_t hc_task_factory_create ( struct ocr_task_factory_struct* factory, ocrEdt_t fctPtr,
                                   u32 paramc, u64 * params, void** paramv, size_t dep_l_size, ocrGuid_t outputEvent) {
    hc_task_t* edt = hc_task_construct(fctPtr, paramc, params, paramv, dep_l_size, outputEvent, factory->task_fct_ptrs);
    ocr_task_t* base = (ocr_task_t*) edt;
    return base->guid;
}

static void hc_task_construct_internal (hc_task_t* derived, ocrEdt_t funcPtr,
        u32 paramc, u64 * params, void** paramv, ocrGuid_t outputEvent, ocr_task_fcts_t * task_fct_ptrs) {
    derived->nbdeps = 0;
    derived->depv = NULL;
    derived->p_function = funcPtr;
    ocr_task_t* base = (ocr_task_t*) derived;
    base->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(base->guid), (u64)base, OCR_GUID_EDT);
    base->paramc = paramc;
    base->params = params;
    base->paramv = paramv;
    base->outputEvent = outputEvent;
    base->fct_ptrs = task_fct_ptrs;
}

hc_task_t* hc_task_construct_with_event_list (ocrEdt_t funcPtr, u32 paramc, u64 * params, void** paramv, event_list_t* el, ocrGuid_t outputEvent, ocr_task_fcts_t * task_fct_ptrs) {
    hc_task_t* derived = (hc_task_t*)checked_malloc(derived, sizeof(hc_task_t));
    derived->awaitList = hc_await_list_constructor_with_event_list(el);
    hc_task_construct_internal(derived, funcPtr, paramc, params, paramv, outputEvent, task_fct_ptrs);
    return derived;
}

hc_task_t* hc_task_construct (ocrEdt_t funcPtr, u32 paramc, u64 * params, void** paramv, size_t dep_list_size, ocrGuid_t outputEvent, ocr_task_fcts_t * task_fct_ptrs) {
    hc_task_t* derived = (hc_task_t*)checked_malloc(derived, sizeof(hc_task_t));
    derived->awaitList = hc_await_list_constructor(dep_list_size);
    hc_task_construct_internal(derived, funcPtr, paramc, params, paramv, outputEvent, task_fct_ptrs);
    return derived;
}

void hc_task_destruct ( ocr_task_t* base ) {
    hc_task_t* derived = (hc_task_t*)base;
    hc_await_list_destructor(derived->awaitList);
    globalGuidProvider->releaseGuid(globalGuidProvider, base->guid);
    free(derived);
}

bool hc_task_iterate_waiting_frontier ( ocr_task_t* base ) {
    hc_task_t* derived = (hc_task_t*)base;
    ocr_event_t** currEventToWaitOn = derived->awaitList->waitingFrontier;

    ocrGuid_t my_guid = base->guid;

    // Try to register current task to events only if they are not ready
    while (*currEventToWaitOn && !(*currEventToWaitOn)->fct_ptrs->registerIfNotReady (*currEventToWaitOn, my_guid) ) {
        // Couldn't register to the event. Event must have been satisfied already.
        // Go on with the task's next event dependency
        ++currEventToWaitOn;
    }
    derived->awaitList->waitingFrontier = currEventToWaitOn;
    return *currEventToWaitOn == NULL;
}

/**
 * @brief Schedules a task when we know its guid.
 * Warning: It does NOT iterate the waiting frontier! The caller
 *          must ensure the task is ready by calling 'iterate_waiting_frontier'
 * Note: This implementation is static to the file, it's only meant to factorize code.
 */
static void hc_task_schedule( ocrGuid_t this_guid, ocr_task_t* base, ocrGuid_t wid ) {
    ocr_worker_t* w = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wid, (u64*)&w, NULL);

    ocr_scheduler_t * scheduler = get_worker_scheduler(w);
    scheduler->give(scheduler, wid, this_guid);
}

/**
 * @brief Tries to schedules a task
 * Iterates the waiting frontier and schedules the task only if
 * all dependencies have been satisfied.
 */
void hc_task_try_schedule( ocr_task_t* base, ocrGuid_t wid ) {
    // Eagerly try to walk the waiting frontier
    if (base->fct_ptrs->iterate_waiting_frontier(base) ) {
        ocrGuid_t this_guid = base->guid;
        hc_task_schedule(this_guid, base, wid);
    }
    // If it's not ready, the task may be enabled at some
    // point when some of its dependencies arrive.
}

void hc_task_execute ( ocr_task_t* base ) {
    hc_task_t* derived = (hc_task_t*)base;
    ocr_event_t* curr = derived->awaitList->array[0];
    ocrDataBlock_t *db = NULL;
    ocrGuid_t dbGuid = UNINITIALIZED_GUID;
    size_t i = 0;
    int nbdeps = derived->nbdeps;
    ocrEdtDep_t * depv = NULL;
    // If any dependencies, acquire their data-blocks
    if (nbdeps != 0) {
        depv = (ocrEdtDep_t *) checked_malloc(depv, sizeof(ocrEdtDep_t) * nbdeps);
        derived->depv = depv;
        while ( NULL != curr ) {
            dbGuid = curr->fct_ptrs->get(curr);
            depv[i].guid = dbGuid;
            //TODO assert this guid is of kind datablock
            if(dbGuid != NULL_GUID) {
                db = NULL;
                globalGuidProvider->getVal(globalGuidProvider, dbGuid, (u64*)&db, NULL);
                depv[i].ptr = db->acquire(db, base->guid, true);
            } else
                depv[i].ptr = NULL;

            curr = derived->awaitList->array[++i];
        }
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

    // Satisfy the output event with the edt's guid
    if (base->outputEvent != NULL_GUID) {
        ocr_event_t * outputEvent;
        globalGuidProvider->getVal(globalGuidProvider, base->outputEvent, (u64*)&outputEvent, NULL);
        outputEvent->fct_ptrs->put(outputEvent, NULL_GUID);
    }
}

void hc_task_add_dependence ( ocr_task_t* base, ocr_event_t* dep, size_t index ) {
    hc_task_t* derived = (hc_task_t*)base;
    derived->awaitList->array[index] = dep;
    derived->nbdeps++;
}
