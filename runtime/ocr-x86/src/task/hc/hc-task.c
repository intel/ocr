/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "debug.h"
#include "event/hc/hc-event.h"
#include "ocr-datablock.h"
#include "ocr-event.h"
#include "ocr-macros.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-task.h"
#include "ocr-utils.h"
#include "ocr-worker.h"
#include "task/hc/hc-task.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include <string.h>

#define SEALED_LIST ((void *) -1)
#define END_OF_LIST NULL
#define UNINITIALIZED_DATA ((ocrGuid_t) -2)

#define DEBUG_TYPE TASK

//
// Convenience functions to get the EDT and deguidify it
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

//
// forward declarations
//

void signalWaiter(ocrGuid_t waiterGuid, ocrGuid_t data, int slot);

void registerSignaler(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot);

void registerWaiter(ocrGuid_t signalerGuid, ocrGuid_t waiterGuid, int slot);

static inline void taskSchedule(ocrGuid_t taskGuid);

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
/* OCR-HC Finish-Latch Utilities                      */
/******************************************************/

// satisfies the incr slot of a finish latch event
static void finishLatchCheckin(ocrEvent_t * base) {
     base->fctPtrs->satisfy(base, NULL_GUID, OCR_EVENT_LATCH_INCR_SLOT);
}

// satisfies the decr slot of a finish latch event
static void finishLatchCheckout(ocrEvent_t * base) {
    base->fctPtrs->satisfy(base, NULL_GUID, OCR_EVENT_LATCH_DECR_SLOT);
}

void setFinishLatch(ocrTask_t * edt, ocrGuid_t latchGuid) {
    edt->els[ELS_SLOT_FINISH_LATCH] = latchGuid;
}

ocrEvent_t * getFinishLatch(ocrTask_t * edt) {
    if (edt != NULL) { //  NULL happens in main when there's no edt yet
        ocrGuid_t latchGuid = edt->els[ELS_SLOT_FINISH_LATCH];
        if (latchGuid != NULL_GUID) {
            ocrEvent_t * event = NULL;
            deguidify(getCurrentPD(), latchGuid, (u64*)&event, NULL);
            return event;
        }
    }
    return NULL;
}

static bool isFinishLatchOwner(ocrEvent_t * finishLatch, ocrGuid_t edtGuid) {
    return (finishLatch != NULL) && (((ocrEventHcFinishLatch_t *)finishLatch)->ownerGuid == edtGuid);
}


/******************************************************/
/* OCR-HC Task Implementation                         */
/******************************************************/

static void newTaskHcInternalCommon (ocrPolicyDomain_t * pd, ocrTaskHc_t* derived,
                                     ocrTaskTemplate_t * taskTemplate, u32 paramc,
                                     u64* paramv, u32 depc, ocrGuid_t outputEvent) {
    if (depc == 0) {
        derived->signalers = END_OF_LIST;
    } else {
        // Since we know how many dependences we have, preallocate signalers
        derived->signalers = checkedMalloc(derived->signalers, sizeof(regNode_t)*depc);
    }
    derived->waiters = END_OF_LIST;
    // Initialize base
    ocrTask_t* base = (ocrTask_t*) derived;
    base->guid = UNINITIALIZED_GUID;
    guidify(pd, (u64)base, &(base->guid), OCR_GUID_EDT);
    base->templateGuid = taskTemplate->guid;
    base->paramc = paramc;
    if(paramc) {
        base->paramv = checkedMalloc(base->paramv, sizeof(u64)*base->paramc);
        memcpy(base->paramv, paramv, sizeof(u64)*base->paramc);
    } else {
        base->paramv = NULL;
    }
    base->outputEvent = outputEvent;
    base->depc = depc;
    base->addedDepCounter = pd->getAtomic64(pd, NULL /*Context*/);
    // Initialize ELS
    int i = 0;
    while (i < ELS_SIZE) {
        base->els[i++] = NULL_GUID;
    }
}

static ocrTaskHc_t* newTaskHcInternal (ocrTaskFactory_t* factory, ocrPolicyDomain_t * pd,
                                       ocrTaskTemplate_t * taskTemplate, u32 paramc,
                                       u64* paramv, u32 depc, u16 properties,
                                       ocrGuid_t affinity, ocrGuid_t outputEvent) {
    ocrTaskHc_t* newEdt = (ocrTaskHc_t*)checkedMalloc(newEdt, sizeof(ocrTaskHc_t));
    newTaskHcInternalCommon(pd, newEdt, taskTemplate, paramc, paramv, depc, outputEvent);
    ocrTask_t * newEdtBase = (ocrTask_t *) newEdt;
    // If we are creating a finish-edt
    if (hasProperty(properties, EDT_PROP_FINISH)) {
        ocrPolicyCtx_t *context = getCurrentWorkerContext();

        ocrGuid_t latchGuid;
        pd->createEvent(pd, &latchGuid, OCR_EVENT_FINISH_LATCH_T,
                        false, context);
        ocrEvent_t *latch = NULL;
        deguidify(pd, latchGuid, (u64*)&latch, NULL);
        ocrEventHcFinishLatch_t * hcLatch = (ocrEventHcFinishLatch_t *) latch;
        // Set the owner of the latch
        hcLatch->ownerGuid = newEdtBase->guid;
        ocrEvent_t * parentLatch = getFinishLatch(getCurrentTask());
        if (parentLatch != NULL) {
            DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on parent flatch 0x%lx\n", newEdtBase->guid, parentLatch->guid);
            // Check in current finish latch
            finishLatchCheckin(parentLatch);
            // Link the new latch to the parent's decrement slot
            // When this finish scope is done, the parent scope is signaled.
            // DESIGN can modify the finish latch to have a standard list of regnode and just addDependence
            hcLatch->parentLatchWaiter.guid = parentLatch->guid;
            hcLatch->parentLatchWaiter.slot = OCR_EVENT_LATCH_DECR_SLOT;
        } else {
            hcLatch->parentLatchWaiter.guid = NULL_GUID;
            hcLatch->parentLatchWaiter.slot = -1;
        }
        DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on self flatch 0x%lx\n", newEdtBase->guid, latch->guid);
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
        ocrEvent_t * curLatch = getFinishLatch(getCurrentTask());
        if (curLatch != NULL) {
            DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on current flatch 0x%lx\n", newEdtBase->guid, curLatch->guid);
            // Check in current finish latch
            finishLatchCheckin(curLatch);
            // Transmit finish-latch to newly created edt
            setFinishLatch(newEdtBase, curLatch->guid);
        }
    }
    return newEdt;
}

static void destructTaskHc ( ocrTask_t* base ) {
    DPRINTF(DEBUG_LVL_INFO, "Destroy 0x%lx\n", base->guid);
    ocrTaskHc_t* derived = (ocrTaskHc_t*)base;
#ifdef FIXME_OCR_ENABLE_STATISTICS
    base->statProcess->fctPtrs->destruct(base->statProcess);
#endif
    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrPolicyCtx_t * orgCtx = getCurrentWorkerContext();
    ocrPolicyCtx_t * ctx = orgCtx->clone(orgCtx);
    ctx->type = PD_MSG_GUID_REL;
    pd->inform(pd, base->guid, ctx);
    base->addedDepCounter->fctPtrs->destruct(base->addedDepCounter);
    ctx->destruct(ctx);
    free(derived);
}

// Signals an edt one of its dependence slot is satisfied
static void taskSignaled(ocrTask_t * base, ocrGuid_t data, u32 slot) {
    // An EDT has a list of signalers, but only register
    // incrementally as signals arrive.
    // Assumption: signal frontier is initialized at slot zero
    // Whenever we receive a signal, it can only be from the
    // current signal frontier, since it is the only signaler
    // the edt is registered with at that time.
    ocrTaskHc_t * self = (ocrTaskHc_t *) base;
    ocrGuid_t signalerGuid = self->signalers[slot].guid;

    if (isEventGuidOfKind(signalerGuid, OCR_EVENT_ONCE_T)) {
        ocrEventHcOnce_t * onceEvent = NULL;
        deguidify(getCurrentPD(), signalerGuid, (u64*)&onceEvent, NULL);
        DPRINTF(DEBUG_LVL_INFO, "Decrement ONCE event reference %lx \n", signalerGuid);
        u64 newNbEdtRegistered = onceEvent->nbEdtRegistered->fctPtrs->xadd(onceEvent->nbEdtRegistered, -1);
        if(newNbEdtRegistered == 0) {
            // deallocate once event
            ocrEvent_t * base = (ocrEvent_t *) onceEvent;
            base->fctPtrs->destruct(base);
        }
    }
    // Replace the signaler's guid by the data guid, this to avoid
    // further references to the event's guid, which is good in general
    // and crucial for once-event since they are being destroyed on satisfy.
    self->signalers[slot].guid = data;
    if (slot == (base->depc-1)) {
        // All dependencies have been satisfied, schedule the edt
        taskSchedule(base->guid);
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
    ASSERT((signalerGuid == NULL_GUID) || isEventGuid(signalerGuid) || isDatablockGuid(signalerGuid));
    //DESIGN would be nice to pre-allocate all of this since we know the edt
    //       dep slot size at edt's creation, then what type should we expose
    //       for the signalers member in the task's data-structure ?
    //No need to CAS anything as all dependencies are provided at creation.
    ocrTaskHc_t * self = (ocrTaskHc_t *) base;
    regNode_t * node = &(self->signalers[slot]);
    node->guid = signalerGuid;
    node->slot = slot;
    // No need to chain nodes here, will use index
    node->next = NULL;
    DPRINTF(DEBUG_LVL_INFO, "AddDependence from 0x%lx to 0x%lx slot %d\n", signalerGuid, base->guid, slot);
}


//
// OCR TASK SCHEDULING
//

/**
 * @brief Schedules a task when we know its guid.
 * Warning: The caller must ensure all dependencies have been satisfied
 * Note: static function only meant to factorize code.
 */
static inline void taskSchedule( ocrGuid_t taskGuid ) {
    DPRINTF(DEBUG_LVL_INFO, "Schedule 0x%lx\n", taskGuid);
    // Setting up the context
    ocrPolicyCtx_t * orgCtx = getCurrentWorkerContext();
    // Current worker schedulers to current policy domain
    // TODO this can be cached somewhere if target/dest are static
    ocrPolicyCtx_t * ctx = orgCtx->clone(orgCtx);
    ctx->destPD = orgCtx->sourcePD;
    ctx->destObj = NULL_GUID;
    ctx->type = PD_MSG_EDT_READY;
    // give the edt to the policy domain
    orgCtx->PD->giveEdt(orgCtx->PD, 1, &taskGuid, ctx);
    ctx->destruct(ctx);
}

/**
 * @brief Tries to schedules a task by registering to its first dependence.
 * If no dependence, schedule the task right-away
 * Warning: This method is to be called ONCE per task and there's no safeguard !
 */
static void tryScheduleTask( ocrTask_t* base ) {
    //DESIGN This is very much specific to the way we've chosen
    //       to handle event-to-task dependence, which needs some
    //       kind of bootstrapping. This could be done when the last
    //       dependence slot is registered with a signaler, but then
    //       the semantic of the schedule function is weaker.
    ocrTaskHc_t* self = (ocrTaskHc_t*)base;
    if (base->depc != 0) {
        // Register the task on the first dependence. If all
        // dependences are here, the task is scheduled by
        // the dependence management code.
        registerWaiter(self->signalers[0].guid, base->guid, 0);
    } else {
        // If there's no dependence, the task must be scheduled now.
        taskSchedule(base->guid);
    }
}

static void taskExecute ( ocrTask_t* base ) {
    DPRINTF(DEBUG_LVL_INFO, "Execute 0x%lx\n", base->guid);
    ocrTaskHc_t* derived = (ocrTaskHc_t*)base;
    // In this implementation each time a signaler has been satisfied, its guid
    // has been replaced by the db guid it has been satisfied with.
    u32 paramc = base->paramc;
    u64 * paramv = base->paramv;
    u64 depc = base->depc;

    ocrEdtDep_t * depv = NULL;
    // If any dependencies, acquire their data-blocks
    if (depc != 0) {
        u64 i = 0;
        //TODO would be nice to resolve regNode into guids before
        depv = (ocrEdtDep_t *) checkedMalloc(depv, sizeof(ocrEdtDep_t) * depc);
        // Double-check we're not rescheduling an already executed edt
        ASSERT(derived->signalers != END_OF_LIST);
        while ( i < depc ) {
            //TODO would be nice to standardize that on satisfy
            regNode_t * regNode = &(derived->signalers[i]);
            ocrGuid_t dbGuid = regNode->guid;
            depv[i].guid = dbGuid;
            if(dbGuid != NULL_GUID) {
                ASSERT(isDatablockGuid(dbGuid));
                ocrDataBlock_t * db = NULL;
                deguidify(getCurrentPD(), dbGuid, (u64*)&db, NULL);
                depv[i].ptr = db->fctPtrs->acquire(db, base->guid, true);
            } else {
                depv[i].ptr = NULL;
            }
            i++;
        }
        free(derived->signalers);
        derived->signalers = END_OF_LIST;
    }

    ocrTaskTemplate_t * taskTemplate;
    deguidify(getCurrentPD(), base->templateGuid, (u64*)&taskTemplate, NULL);
    //TODO: define when task template is resolved from its guid
    ocrGuid_t retGuid = taskTemplate->executePtr(paramc, paramv,
                                                 depc, depv);

    // edt user code is done, if any deps, release data-blocks
    if (depc != 0) {
        u64 i = 0;
        for(i=0; i<depc; ++i) {
            if(depv[i].guid != NULL_GUID) {
                ocrDataBlock_t * db = NULL;
                deguidify(getCurrentPD(), depv[i].guid, (u64*)&db, NULL);
                RESULT_ASSERT(db->fctPtrs->release(db, base->guid, true), ==, 0);
            }
        }
        free(depv);
    }
    if(paramv)
        free(paramv); // Free the parameter array (we copied them in)
    bool satisfyOutputEvent = (base->outputEvent != NULL_GUID);
    // check out from current finish scope
    ocrEvent_t * curLatch = getFinishLatch(base);
    if (curLatch != NULL) {
        // if own the latch, then set the retGuid
        if (isFinishLatchOwner(curLatch, base->guid)) {
            ((ocrEventHcFinishLatch_t *) curLatch)->returnGuid = retGuid;
            satisfyOutputEvent = false;
        }
        // If the edt is the last to checkout from the current finish scope,
        // the latch event automatically satisfies the parent latch (if any)
        // and the output event associated with the current finish-edt (if any)
        DPRINTF(DEBUG_LVL_INFO, "Checkout 0x%lx on flatch 0x%lx\n", base->guid, curLatch->guid);
        finishLatchCheckout(curLatch);
    }

    // !! Warning: curLatch might have been deallocated at that point !!

    if (satisfyOutputEvent) {
        ocrEvent_t * outputEvent;
        deguidify(getCurrentPD(), base->outputEvent, (u64*)&outputEvent, NULL);
        // We know the output event must be of type single sticky since it is
        // internally allocated by the runtime
        ASSERT(isEventSingleGuid(base->outputEvent));
        outputEvent->fctPtrs->satisfy(outputEvent, retGuid, 0);
    }
}

/******************************************************/
/* OCR-HC Task Template Factory                       */
/******************************************************/

static void destructTaskTemplateHc(ocrTaskTemplate_t *self) {
    free(self);
}

static ocrTaskTemplate_t * newTaskTemplateHc(ocrTaskTemplateFactory_t* factory,
                                      ocrEdt_t executePtr, u32 paramc, u32 depc, ocrParamList_t *perInstance) {
    ocrTaskTemplateHc_t* template = (ocrTaskTemplateHc_t*) checkedMalloc(template, sizeof(ocrTaskTemplateHc_t));
    ocrTaskTemplate_t * base = (ocrTaskTemplate_t *) template;
    base->paramc = paramc;
    base->depc = depc;
    base->executePtr = executePtr;
    base->guid = UNINITIALIZED_GUID;
    base->fctPtrs = &(factory->taskTemplateFcts);
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_EDT_TEMPLATE);
    return base;
}

static void destructTaskTemplateFactoryHc(ocrTaskTemplateFactory_t* base) {
    ocrTaskTemplateFactoryHc_t* derived = (ocrTaskTemplateFactoryHc_t*) base;
    free(derived);
}

ocrTaskTemplateFactory_t * newTaskTemplateFactoryHc(ocrParamList_t* perType) {
    ocrTaskTemplateFactoryHc_t* derived = (ocrTaskTemplateFactoryHc_t*) checkedMalloc(derived, sizeof(ocrTaskTemplateFactoryHc_t));
    ocrTaskTemplateFactory_t* base = (ocrTaskTemplateFactory_t*) derived;
    base->instantiate = newTaskTemplateHc;
    base->destruct =  destructTaskTemplateFactoryHc;
    base->taskTemplateFcts.destruct = &destructTaskTemplateHc;
    //TODO What taskTemplateFcts is supposed to do ?
    return base;
}

/******************************************************/
/* OCR-HC Task Factory                                */
/******************************************************/

ocrTask_t * newTaskHc(ocrTaskFactory_t* factory, ocrTaskTemplate_t * taskTemplate,
                      u32 paramc, u64* paramv, u32 depc, u16 properties,
                      ocrGuid_t affinity, ocrGuid_t * outputEventPtr,
                      ocrParamList_t *perInstance) {

    // Initialize a sticky outputEvent if requested (ptr not NULL)
    // i.e. the user gave a place-holder for the runtime to initialize and set an output event.
    ocrGuid_t outputEvent = (ocrGuid_t) outputEventPtr;
    ocrPolicyDomain_t* pd = getCurrentPD();
    if (outputEvent != NULL_GUID) {
        ocrPolicyCtx_t *context = getCurrentWorkerContext();
        pd->createEvent(pd, outputEventPtr, OCR_EVENT_STICKY_T, false,
                        context);
        outputEvent = *outputEventPtr;
    }
    ocrTaskHc_t* edt = newTaskHcInternal(factory, pd, taskTemplate, paramc, paramv,
                                         depc, properties, affinity, outputEvent);
    ocrTask_t* base = (ocrTask_t*) edt;
    base->fctPtrs = &(factory->taskFcts);
    DPRINTF(DEBUG_LVL_INFO, "Create 0x%lx depc %d outputEvent 0x%lx\n", base->guid, depc, outputEvent);
    return base;
}

void destructTaskFactoryHc(ocrTaskFactory_t* base) {
    ocrTaskFactoryHc_t* derived = (ocrTaskFactoryHc_t*) base;
    free(derived);
}

ocrTaskFactory_t * newTaskFactoryHc(ocrParamList_t* perInstance) {
    ocrTaskFactoryHc_t* derived = (ocrTaskFactoryHc_t*) checkedMalloc(derived, sizeof(ocrTaskFactoryHc_t));
    ocrTaskFactory_t* base = (ocrTaskFactory_t*) derived;
    base->instantiate = newTaskHc;
    base->destruct =  destructTaskFactoryHc;
    // initialize singleton instance that carries hc implementation
    // function pointers. Every instantiated task template will use
    // this pointer to resolve functions implementations.
    base->taskFcts.destruct = destructTaskHc;
    base->taskFcts.execute = taskExecute;
    base->taskFcts.schedule = tryScheduleTask;
    return base;
}


/******************************************************/
/* Signal/Wait interface implementation               */
/******************************************************/

// Registers a 'waiter' that should be signaled by 'self' on a particular slot.
// If 'self' has already been satisfied, it signals 'waiter' right away.
// Warning: Concurrent with  event's put.
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
    if (isEventGuid(signalerGuid) && isEventGuid(waiterGuid)) {
        ocrEventHcAwaitable_t * target;
        deguidify(getCurrentPD(), signalerGuid, (u64*)&target, NULL);
        awaitableEventRegisterWaiter(target, waiterGuid, slot);
        return;
    }
    // ONCE event must know who are consuming them so that
    // they are not deallocated prematurely
    if (isEventGuidOfKind(signalerGuid, OCR_EVENT_ONCE_T) && isEdtGuid(waiterGuid)) {
        ocrEvent_t * signalerEvent;
        deguidify(getCurrentPD(), signalerGuid, (u64*)&signalerEvent, NULL);
        onceEventRegisterEdtWaiter(signalerEvent, waiterGuid, slot);
    }

    // SIGNAL MODE:
    //  - anything-to-edt registration
    //  - db-to-event registration
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
    if (isEdtGuid(waiterGuid)) {
        // edt waiting for a signal from an event or a datablock
        ASSERT((signalerGuid == NULL_GUID) || isEventGuid(signalerGuid) || isDatablockGuid(signalerGuid));
        ocrTask_t * target = NULL;

        deguidify(getCurrentPD(), waiterGuid, (u64*)&target, NULL);
        edtRegisterSignaler(target, signalerGuid, slot);
        if ( target->depc == target->addedDepCounter->fctPtrs->xadd(target->addedDepCounter,1) ) {
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
