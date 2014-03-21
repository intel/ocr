/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#if defined(ENABLE_TASK_HC) || defined(ENABLE_TASKTEMPLATE_HC)

#include "debug.h"
#include "event/hc/hc-event.h"
#include "ocr-datablock.h"
#include "ocr-event.h"
#include "ocr-errors.h"
#include "ocr-hal.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-task.h"
#include "ocr-worker.h"
#include "task/hc/hc-task.h"
#include "utils/ocr-utils.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#ifdef OCR_ENABLE_PROFILING_STATISTICS
#endif 
#endif /* OCR_ENABLE_STATISTICS */

#define DEBUG_TYPE TASK

/******************************************************/
/* OCR-HC Task Template Factory                       */
/******************************************************/

#ifdef ENABLE_TASKTEMPLATE_HC

u8 destructTaskTemplateHc(ocrTaskTemplate_t *self) {
#ifdef OCR_ENABLE_STATISTICS
    {
        // TODO: FIXME
        ocrPolicyDomain_t *pd = getCurrentPD();
        ocrGuid_t edtGuid = getCurrentEDT();
                
        statsTEMP_DESTROY(pd, edtGuid, NULL, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */
    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid.guid) = self->guid;
    PD_MSG_FIELD(guid.metaDataPtr) = self;
    PD_MSG_FIELD(properties) = 1;
    RESULT_PROPAGATE(pd->processMessage(pd, &msg, false));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

ocrTaskTemplate_t * newTaskTemplateHc(ocrTaskTemplateFactory_t* factory, ocrEdt_t executePtr,
                                      u32 paramc, u32 depc, const char* fctName,
                                      ocrParamList_t *perInstance) {

    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;

    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_CREATE
    msg.type = PD_MSG_GUID_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = NULL_GUID;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    PD_MSG_FIELD(size) = sizeof(ocrTaskTemplateHc_t);
    PD_MSG_FIELD(kind) = OCR_GUID_EDT_TEMPLATE;
    PD_MSG_FIELD(properties) = 0;

    RESULT_PROPAGATE2(pd->processMessage(pd, &msg, true), NULL);

    ocrTaskTemplate_t *base = (ocrTaskTemplate_t*)PD_MSG_FIELD(guid.metaDataPtr);
    ASSERT(base);
    base->guid = PD_MSG_FIELD(guid.guid);
#undef PD_MSG
#undef PD_TYPE
    
    base->paramc = paramc;
    base->depc = depc;
    base->executePtr = executePtr;
#ifdef OCR_ENABLE_EDT_NAMING
    base->name = fctName;
#endif
    base->fctId = factory->factoryId;
    
#ifdef OCR_ENABLE_STATISTICS
    {
        // TODO: FIXME
        ocrGuid_t edtGuid = getCurrentEDT();
        statsTEMP_CREATE(pd, edtGuid, NULL, base->guid, base);
    }
#endif /* OCR_ENABLE_STATISTICS */
    return base;
}

void destructTaskTemplateFactoryHc(ocrTaskTemplateFactory_t* base) {
    runtimeChunkFree((u64)base, NULL);
}

ocrTaskTemplateFactory_t * newTaskTemplateFactoryHc(ocrParamList_t* perType, u32 factoryId) {
    ocrTaskTemplateFactory_t* base = (ocrTaskTemplateFactory_t*)runtimeChunkAlloc(sizeof(ocrTaskTemplateFactoryHc_t), NULL);
    
    base->instantiate = FUNC_ADDR(ocrTaskTemplate_t* (*)(ocrTaskTemplateFactory_t*, ocrEdt_t, u32, u32, const char*, ocrParamList_t*), newTaskTemplateHc);
    base->destruct =  FUNC_ADDR(void (*)(ocrTaskTemplateFactory_t*), destructTaskTemplateFactoryHc);
    base->factoryId = factoryId;
    base->fcts.destruct = FUNC_ADDR(u8 (*)(ocrTaskTemplate_t*), destructTaskTemplateHc);
    return base;
}

#endif /* ENABLE_TASKTEMPLATE_HC */

#ifdef ENABLE_TASK_HC

/******************************************************/
/* OCR HC latch utilities                             */
/******************************************************/

static ocrFatGuid_t getFinishLatch(ocrTask_t * edt) {
    ocrFatGuid_t result = {.guid = NULL_GUID, .metaDataPtr = NULL};
    if (edt != NULL) { //  NULL happens in main when there's no edt yet
        if(edt->finishLatch)
            result.guid = edt->finishLatch;
        else
            result.guid = edt->parentLatch;
    }
    return result;
}

// satisfies the incr slot of a finish latch event
static u8 finishLatchCheckin(ocrPolicyDomain_t *pd, ocrPolicyMsg_t *msg,
                               ocrFatGuid_t sourceEvent, ocrFatGuid_t latchEvent) {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DEP_SATISFY
    msg->type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid) = latchEvent;
    PD_MSG_FIELD(payload.guid) = NULL_GUID;
    PD_MSG_FIELD(payload.metaDataPtr) = NULL;
    PD_MSG_FIELD(slot) = OCR_EVENT_LATCH_INCR_SLOT;
    PD_MSG_FIELD(properties) = 0;
    RESULT_PROPAGATE(pd->processMessage(pd, msg, false));
#undef PD_TYPE
#define PD_TYPE PD_MSG_DEP_ADD
    msg->type = PD_MSG_DEP_ADD | PD_MSG_REQUEST;
    PD_MSG_FIELD(source) = sourceEvent;
    PD_MSG_FIELD(dest) = latchEvent;
    PD_MSG_FIELD(slot) = OCR_EVENT_LATCH_DECR_SLOT;
    PD_MSG_FIELD(properties) = 0; // TODO: do we want a mode for this?
    RESULT_PROPAGATE(pd->processMessage(pd, msg, false));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

/******************************************************/
/* Random helper functions                            */
/******************************************************/

static inline bool hasProperty(u32 properties, u32 property) {
    return properties & property;
}

static u8 registerOnFrontier(ocrTaskHc_t *self, ocrPolicyDomain_t *pd,
                               ocrPolicyMsg_t *msg, u32 slot) {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DEP_REGWAITER
    msg->type = PD_MSG_DEP_REGWAITER | PD_MSG_REQUEST;
    PD_MSG_FIELD(waiter.guid) = self->base.guid;
    PD_MSG_FIELD(waiter.metaDataPtr) = self;
    PD_MSG_FIELD(dest.guid) = self->signalers[slot].guid;
    PD_MSG_FIELD(dest.metaDataPtr) = NULL;
    PD_MSG_FIELD(slot) = self->signalers[slot].slot;
    PD_MSG_FIELD(properties) = 0;
    RESULT_PROPAGATE(pd->processMessage(pd, msg, false));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}
/******************************************************/
/* OCR-HC Support functions                           */
/******************************************************/

static u8 initTaskHcInternal(ocrTaskHc_t *task, ocrPolicyDomain_t * pd,
                               ocrTask_t *curTask, ocrFatGuid_t outputEvent,
                               ocrFatGuid_t parentLatch, u32 properties) {

    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);

    task->frontierSlot = 0;
    task->slotSatisfiedCount = 0;
    task->lock = 0;
    task->unkDbs = NULL;
    task->countUnkDbs = 0;
    task->maxUnkDbs = 0;
    
    if(task->base.depc == 0) {
        task->signalers = END_OF_LIST;
    }
    // If we are creating a finish-edt
    if (hasProperty(properties, EDT_PROP_FINISH)) {
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EVT_CREATE
        msg.type = PD_MSG_EVT_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD(guid.guid) = NULL_GUID;
        PD_MSG_FIELD(guid.metaDataPtr) = NULL;
        PD_MSG_FIELD(type) = OCR_EVENT_LATCH_T;
        PD_MSG_FIELD(properties) = 0;
        RESULT_PROPAGATE(pd->processMessage(pd, &msg, true));

        ocrFatGuid_t latchFGuid = PD_MSG_FIELD(guid);
#undef PD_MSG
#undef PD_TYPE
        ASSERT(latchFGuid.guid != NULL_GUID && latchFGuid.metaDataPtr != NULL);
        
        if (parentLatch.guid != NULL_GUID) {
            DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on parent flatch 0x%lx\n", task->base.guid, parentLatch.guid);
            // Check in current finish latch
            RESULT_PROPAGATE(finishLatchCheckin(pd, &msg, latchFGuid, parentLatch));
        }
        
        DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on self flatch 0x%lx\n", task->base.guid, latchFGuid.guid);
        // Check in the new finish scope
        // This will also link outputEvent to latchFGuid
        RESULT_PROPAGATE(finishLatchCheckin(pd, &msg, outputEvent, latchFGuid));
        // Set edt's ELS to the new latch
        task->base.finishLatch = latchFGuid.guid;
    } else {
        // If the currently executing edt is in a finish scope,
        // but is not a finish-edt itself, just register to the scope
        if(parentLatch.guid != NULL_GUID) {
            DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on current flatch 0x%lx\n", task->base.guid, parentLatch.guid);
            // Check in current finish latch
            RESULT_PROPAGATE(finishLatchCheckin(pd, &msg, outputEvent, parentLatch));
        }
    }
    return 0;
}

/**
 * @brief Schedules a task.
 * Warning: The caller must ensure all dependencies have been satisfied
 * Note: static function only meant to factorize code.
 */
static u8 taskSchedule(ocrTask_t *self) {
    DPRINTF(DEBUG_LVL_INFO, "Schedule 0x%lx\n", self->guid);

    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);

    ocrFatGuid_t toGive = {.guid = self->guid, .metaDataPtr = self};

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_COMM_GIVE
    msg.type = PD_MSG_COMM_GIVE | PD_MSG_REQUEST;
    PD_MSG_FIELD(guids) = &toGive;
    PD_MSG_FIELD(guidCount) = 1;
    PD_MSG_FIELD(properties) = 0;
    PD_MSG_FIELD(type) = OCR_GUID_EDT;
    RESULT_PROPAGATE(pd->processMessage(pd, &msg, false));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

/******************************************************/
/* OCR-HC Task Implementation                         */
/******************************************************/

u8 destructTaskHc(ocrTask_t* base) {
    DPRINTF(DEBUG_LVL_INFO, "Destroy 0x%lx\n", base->guid);

    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#ifdef OCR_ENABLE_STATISTICS
    {
        // TODO: FIXME
        // An EDT is destroyed just when it finishes running so
        // the source is basically itself
        statsEDT_DESTROY(pd, base->guid, base, base->guid, base);
    }
#endif /* OCR_ENABLE_STATISTICS */
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    // These next two statements may be not required. Just to be safe
    PD_MSG_FIELD(guid.guid) = base->guid;
    PD_MSG_FIELD(guid.metaDataPtr) = base;
    PD_MSG_FIELD(properties) = 1; // Free metadata
    RESULT_PROPAGATE(pd->processMessage(pd, &msg, false));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

ocrTask_t * newTaskHc(ocrTaskFactory_t* factory, ocrFatGuid_t edtTemplate,
                      u32 paramc, u64* paramv, u32 depc, u32 properties,
                      ocrFatGuid_t affinity, ocrFatGuid_t * outputEventPtr,
                      ocrParamList_t *perInstance) {

    // Get the current environment
    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    ocrTask_t *curEdt = NULL;
    u32 i;

    getCurrentEnv(&pd, NULL, &curEdt, &msg);

    ocrFatGuid_t parentLatch = getFinishLatch(curEdt);
    ocrFatGuid_t outputEvent = {.guid = NULL_GUID, .metaDataPtr = NULL};
    // We need an output event for the EDT if either:
    //  - the user requested one (outputEventPtr is non NULL)
    //  - the EDT is a finish EDT (and therefore we need to link
    //    the output event to the latch event)
    //  - the EDT is within a finish scope (and we need to link to
    //    that latch event)
    if (outputEventPtr != NULL || hasProperty(properties, EDT_PROP_FINISH) ||
        parentLatch.guid != NULL_GUID) {
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EVT_CREATE
        msg.type = PD_MSG_EVT_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD(guid.guid) = NULL_GUID;
        PD_MSG_FIELD(guid.metaDataPtr) = NULL;
        PD_MSG_FIELD(properties) = 0;
        PD_MSG_FIELD(type) = OCR_EVENT_ONCE_T; // Output events of EDTs are non sticky

        RESULT_PROPAGATE2(pd->processMessage(pd, &msg, true), NULL);
        outputEvent = PD_MSG_FIELD(guid);
        
#undef PD_MSG
#undef PD_TYPE
    }

    // Create the task itself by getting a GUID
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_CREATE
    msg.type = PD_MSG_GUID_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = NULL_GUID;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    // We allocate everything in the meta-data to keep things simple
    PD_MSG_FIELD(size) = sizeof(ocrTaskHc_t) + paramc*sizeof(u64) + depc*sizeof(regNode_t);
    PD_MSG_FIELD(kind) = OCR_GUID_EDT;
    PD_MSG_FIELD(properties) = 0;
    RESULT_PROPAGATE2(pd->processMessage(pd, &msg, true), NULL);
    ocrTaskHc_t *edt = (ocrTaskHc_t*)PD_MSG_FIELD(guid.metaDataPtr);
    ocrTask_t *base = (ocrTask_t*)edt;
    ASSERT(edt);

    // Set-up base structures
    base->guid = PD_MSG_FIELD(guid.guid);
    base->templateGuid = edtTemplate.guid;
    ASSERT(edtTemplate.metaDataPtr); // For now we just assume it is passed whole
    base->funcPtr = ((ocrTaskTemplate_t*)(edtTemplate.metaDataPtr))->executePtr;
    base->paramv = (u64*)((u64)base + sizeof(ocrTaskHc_t));
#ifdef OCR_ENABLE_EDT_NAMING
    base->name = ((ocrTaskTemplate_t*)(edtTemplate.metaDataPtr))->name;
#endif
    base->outputEvent = outputEvent.guid;
    base->finishLatch = NULL_GUID;
    base->parentLatch = parentLatch.guid;
    for(i = 0; i < ELS_SIZE; ++i) {
        base->els[i] = NULL_GUID;
    }
    base->state = CREATED_EDTSTATE;
    base->paramc = paramc;
    base->depc = depc;
    base->fctId = factory->factoryId;
    for(i = 0; i < paramc; ++i) {
        base->paramv[i] = paramv[i];
    }
    
    edt->signalers = (regNode_t*)((u64)edt + sizeof(ocrTaskHc_t) + paramc*sizeof(u64));
    // Initialize the signalers properly
    for(i = 0; i < depc; ++i) {
        edt->signalers[i].guid = UNINITIALIZED_GUID;
        edt->signalers[i].slot = i;
    }
    
    // Set up HC specific stuff
    RESULT_PROPAGATE2(initTaskHcInternal(edt, pd, curEdt, outputEvent, parentLatch, properties), NULL);

    // Set up outputEventPtr:
    //   - if a finish EDT, wait on its latch event
    //   - if not a finish EDT, wait on its output event
    if(outputEventPtr) {
        if(base->finishLatch) {
            outputEventPtr->guid = base->finishLatch;
        } else {
            outputEventPtr->guid = base->outputEvent;
        }
    }
#undef PD_MSG
#undef PD_TYPE
    
#ifdef OCR_ENABLE_STATISTICS
    // TODO FIXME
    {
        ocrGuid_t edtGuid = getCurrentEDT();
        if(edtGuid) {
            // Usual case when the EDT is created within another EDT
            ocrTask_t *task = NULL;
            deguidify(pd, edtGuid, (u64*)&task, NULL);

            statsTEMP_USE(pd, edtGuid, task, taskTemplate->guid, taskTemplate);
            statsEDT_CREATE(pd, edtGuid, task, base->guid, base);
        } else {
            statsTEMP_USE(pd, edtGuid, NULL, taskTemplate->guid, taskTemplate);
            statsEDT_CREATE(pd, edtGuid, NULL, base->guid, base);
        }
    }
#endif /* OCR_ENABLE_STATISTICS */
    DPRINTF(DEBUG_LVL_INFO, "Create 0x%lx depc %d outputEvent 0x%lx\n", base->guid, depc, outputEventPtr?outputEventPtr->guid:NULL_GUID);

    // Check to see if the EDT can be run
    if(base->depc == edt->slotSatisfiedCount) {
        DPRINTF(DEBUG_LVL_VVERB, "Scheduling task 0x%lx due to initial satisfactions\n",
                base->guid);
        RESULT_PROPAGATE2(taskSchedule(base), NULL);
    }
    return base;
}

u8 satisfyTaskHc(ocrTask_t * base, ocrFatGuid_t data, u32 slot) {
    // An EDT has a list of signalers, but only registers
    // incrementally as signals arrive AND on non-persistent
    // events (latch or ONCE)
    // Assumption: signal frontier is initialized at slot zero
    // Whenever we receive a signal:
    //  - it can be from the frontier (we registered on it)
    //  - it can be a ONCE event
    //  - it can be a data-block being added (causing an immediate
    //    satisfy
    ocrTaskHc_t * self = (ocrTaskHc_t *) base;

    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);
    
    // Could be moved a little later if the ASSERT was not here
    // Should not make a huge difference
    hal_lock32(&(self->lock));
    ASSERT(slot < base->depc); // Check to make sure non crazy value is passed in
    ASSERT(self->signalers[slot].slot != (u32)-1); // Check to see if not already satisfied
    ASSERT((self->signalers[slot].slot == slot && (self->signalers[slot].slot == self->frontierSlot)) ||
           (self->signalers[slot].slot == (u32)-2) || /* Checks if ONCE/LATCH event satisfaction */
           (slot > self->frontierSlot)); /* A DB or NULL_GUID being added as ocrAddDependence */
                                                   
    self->signalers[slot].guid = data.guid;
    self->signalers[slot].slot = (u32)-1; // Say that it is satisfied
    if(++self->slotSatisfiedCount == base->depc) {
        ++(self->slotSatisfiedCount); // So others don't catch the satisfaction
        hal_unlock32(&(self->lock));
        // All dependences have been satisfied, schedule the edt
        DPRINTF(DEBUG_LVL_VERB, "Scheduling task 0x%lx due to last satisfied dependence\n",
                self->base.guid);
        RESULT_PROPAGATE(taskSchedule(base));
    } else if(self->frontierSlot == slot) {
        // We need to go register to the next non-once dependence
        while(++self->frontierSlot < base->depc &&
              self->signalers[self->frontierSlot].slot != ++slot) ;
        // We found a slot that is == to slot (so unsatisfied and not once)
        if(self->frontierSlot < base->depc &&
           self->signalers[self->frontierSlot].guid != UNINITIALIZED_GUID) {
            u32 tslot = self->frontierSlot;
            hal_unlock32(&(self->lock));
            RESULT_PROPAGATE(registerOnFrontier(self, pd, &msg, tslot));
            // If we are UNITIALIZED_GUID, we will do the REGWAITER
            // when we do get the dependence
        } else {
            hal_unlock32(&(self->lock));
        }
    } else {
        hal_unlock32(&(self->lock));
    }
    return 0;
}

u8 registerSignalerTaskHc(ocrTask_t * base, ocrFatGuid_t signalerGuid, u32 slot, bool isDepAdd) {
    ASSERT(isDepAdd); // This should only be called when adding a dependence
    
    ocrTaskHc_t * self = (ocrTaskHc_t *) base;
    regNode_t * node = &(self->signalers[slot]);
    
    node->guid = signalerGuid.guid;

    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);
    
    ocrGuidKind signalerKind = OCR_GUID_NONE;
    deguidify(pd, &signalerGuid, &signalerKind);
    if(signalerKind == OCR_GUID_EVENT) {
        node->slot = slot;
        ocrEventTypes_t evtKind = eventType(pd, signalerGuid);
        if(evtKind == OCR_EVENT_ONCE_T ||
           evtKind == OCR_EVENT_LATCH_T) {

            node->slot = (u32)-2; // To signal that this is a once event
            
            // We need to move the frontier slot over
            hal_lock32(&(self->lock));
            while(++self->frontierSlot < base->depc &&
                  self->signalers[self->frontierSlot].slot != ++slot) ;
            
            // We found a slot that is == to slot (so unsatisfied and not once)
            if(self->frontierSlot < base->depc &&
               self->signalers[self->frontierSlot].guid != UNINITIALIZED_GUID) {
                u32 tslot = self->frontierSlot;
                hal_unlock32(&(self->lock));
                
                RESULT_PROPAGATE(registerOnFrontier(self, pd, &msg, tslot));
                // If we are UNITIALIZED_GUID, we will do the REGWAITER
                // when we add the dependence (just below)
            } else {
                hal_unlock32(&(self->lock));
            }   
        } else {
            if(slot == self->frontierSlot) {
                // We actually need to register ourself as a waiter here
                ocrPolicyDomain_t *pd = NULL;
                ocrPolicyMsg_t msg;
                getCurrentEnv(&pd, NULL, NULL, &msg);
                RESULT_PROPAGATE(registerOnFrontier(self, pd, &msg, slot));
            }
        }
    } else {
        if(signalerKind == OCR_GUID_DB) {
            node->slot = (u32)-1; // Already satisfied
            hal_lock32(&(self->lock));
            ++(self->slotSatisfiedCount);
            if(base->depc == self->slotSatisfiedCount) {
                ++(self->slotSatisfiedCount); // We make it go one over to not schedule twice
                hal_unlock32(&(self->lock));
                DPRINTF(DEBUG_LVL_VERB, "Scheduling task 0x%lx due to an add dependence\n",
                        self->base.guid);
                RESULT_PROPAGATE(taskSchedule(base));
            } else {
                hal_unlock32(&(self->lock));
            }
        } else {
            ASSERT(0);
        }
    }
    
    DPRINTF(DEBUG_LVL_INFO, "AddDependence from 0x%lx to 0x%lx slot %d\n", signalerGuid.guid, base->guid, slot);
    return 0;
}

u8 unregisterSignalerTaskHc(ocrTask_t * base, ocrFatGuid_t signalerGuid, u32 slot, bool isDepRem) {
    ASSERT(0); // We don't support this at this time...
    return 0;
}

u8 notifyDbAcquireTaskHc(ocrTask_t *base, ocrFatGuid_t db) {
    // This implementation does NOT support EDTs moving while they are executing
    ocrTaskHc_t *derived = (ocrTaskHc_t*)base;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, NULL);
    if(derived->countUnkDbs == 0) {
        derived->unkDbs = (ocrGuid_t*)pd->pdMalloc(pd, sizeof(ocrGuid_t)*8);
        ASSERT(derived->unkDbs);
        derived->maxUnkDbs = 8;
    } else {
        if(derived->maxUnkDbs == derived->countUnkDbs) {
            ocrGuid_t *oldPtr = derived->unkDbs;
            derived->unkDbs = (ocrGuid_t*)pd->pdMalloc(pd, sizeof(ocrGuid_t)*derived->maxUnkDbs*2);
            ASSERT(derived->unkDbs);
            hal_memCopy(derived->unkDbs, oldPtr, sizeof(ocrGuid_t)*derived->maxUnkDbs, false);
            pd->pdFree(pd, oldPtr);
            derived->maxUnkDbs *= 2;
        }
    }
    // Tack on this DB
    derived->unkDbs[derived->countUnkDbs] = db.guid;
    ++derived->countUnkDbs;
    DPRINTF(DEBUG_LVL_VERB, "EDT (GUID: 0x%lx) added DB (GUID: 0x%lx) to its list of dyn. acquired DBs (have %d)\n",
            base->guid, db.guid, derived->countUnkDbs);
    return 0;
}

u8 notifyDbReleaseTaskHc(ocrTask_t *base, ocrFatGuid_t db) {
    ocrTaskHc_t *derived = (ocrTaskHc_t*)base;
    if(derived->unkDbs == NULL)
        return 0; // May be a release we don't care about
    u64 maxCount = derived->countUnkDbs;
    u64 count = 0;
    DPRINTF(DEBUG_LVL_VERB, "Notifying EDT (GUID: 0x%lx) that it acquired db (GUID: 0x%lx)\n",
            base->guid, db.guid);
    while(count < maxCount) {
        // We bound our search (in case there is an error)
        if(db.guid == derived->unkDbs[count]) {
            DPRINTF(DEBUG_LVL_VVERB, "Found a match for count %lu\n", count);
            derived->unkDbs[count] = derived->unkDbs[maxCount - 1];
            --(derived->countUnkDbs);
            return 0;
        }
        ++count;
    }
    // We did not find it but it may be that we never acquired it
    // Should not be an error code
    return 0;
}

u8 taskExecute(ocrTask_t* base) {
    DPRINTF(DEBUG_LVL_INFO, "Execute 0x%lx\n", base->guid);
    ocrTaskHc_t* derived = (ocrTaskHc_t*)base;
    // In this implementation each time a signaler has been satisfied, its guid
    // has been replaced by the db guid it has been satisfied with.
    u32 paramc = base->paramc;
    u64 * paramv = base->paramv;
    u32 depc = base->depc;

    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);

    ocrEdtDep_t * depv = NULL;
    // If any dependencies, acquire their data-blocks
    u32 maxAcquiredDb = 0;
    
    ASSERT(derived->unkDbs == NULL); // Should be no dynamically acquired DBs before running
    
    if (depc != 0) {
        //TODO would be nice to resolve regNode into guids before
        depv = pd->pdMalloc(pd, sizeof(ocrEdtDep_t)*depc);
        // Double-check we're not rescheduling an already executed edt
        ASSERT(derived->signalers != END_OF_LIST);
        // Make sure the task was actually fully satisfied
        ASSERT(derived->slotSatisfiedCount == depc+1);
        while( maxAcquiredDb < depc ) {
            //TODO would be nice to standardize that on satisfy
            depv[maxAcquiredDb].guid = derived->signalers[maxAcquiredDb].guid;
            if(depv[maxAcquiredDb].guid != NULL_GUID) {
                // We send a message that we want to acquire the DB
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
                msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
                PD_MSG_FIELD(guid.guid) = depv[maxAcquiredDb].guid;
                PD_MSG_FIELD(guid.metaDataPtr) = NULL;
                PD_MSG_FIELD(edt.guid) = base->guid;
                PD_MSG_FIELD(edt.metaDataPtr) = base;
                PD_MSG_FIELD(properties) = 1; // Runtime acquire
                // This call may fail if the policy domain goes down
                // while we are staring to execute
                
                if(pd->processMessage(pd, &msg, true)) {
                    // We are not going to launch the EDT
                    break;
                }
                depv[maxAcquiredDb].ptr = PD_MSG_FIELD(ptr);
#undef PD_MSG
#undef PD_TYPE
            } else {
                depv[maxAcquiredDb].ptr = NULL;
            }
            ++maxAcquiredDb;
        }
        derived->signalers = END_OF_LIST;
    }
    
#ifdef OCR_ENABLE_STATISTICS
    // TODO: FIXME
    ocrPolicyCtx_t *ctx = getCurrentWorkerContext();
    ocrWorker_t *curWorker = NULL;
    
    deguidify(pd, ctx->sourceObj, (u64*)&curWorker, NULL);
    
    // We first have the message of using the EDT Template
    statsTEMP_USE(pd, base->guid, base, taskTemplate->guid, taskTemplate);
    
    // We now say that the worker is starting the EDT
    statsEDT_START(pd, ctx->sourceObj, curWorker, base->guid, base, depc != 0);
    
#endif /* OCR_ENABLE_STATISTICS */

    ocrGuid_t retGuid = NULL_GUID;
    if(depc == 0 || (maxAcquiredDb == depc)) {
        retGuid = base->funcPtr(paramc, paramv, depc, depv);
    }
#ifdef OCR_ENABLE_STATISTICS
    // We now say that the worker is done executing the EDT
    statsEDT_END(pd, ctx->sourceObj, curWorker, base->guid, base);
#endif /* OCR_ENABLE_STATISTICS */

    // edt user code is done, if any deps, release data-blocks
    if(depc != 0) {
        u32 i;
        for(i=0; i < maxAcquiredDb; ++i) { // Only release the ones we managed to grab
            if(depv[i].guid != NULL_GUID) {
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_RELEASE
                msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST;
                PD_MSG_FIELD(guid.guid) = depv[i].guid;
                PD_MSG_FIELD(guid.metaDataPtr) = NULL;
                PD_MSG_FIELD(edt.guid) = base->guid;
                PD_MSG_FIELD(edt.metaDataPtr) = base;
                PD_MSG_FIELD(properties) = 1; // Runtime release
                // Ignore failures at this point
                pd->processMessage(pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
            }
        }
    }

    // We now release all other data-blocks that we may potentially
    // have acquired along the way
    if(derived->unkDbs != NULL) {
        // We acquire this DB
        ocrGuid_t *extraToFree = derived->unkDbs;
        u64 count = derived->countUnkDbs;
        while(count) {
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_RELEASE
            msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST;
            PD_MSG_FIELD(guid.guid) = extraToFree[0];
            PD_MSG_FIELD(guid.metaDataPtr) = NULL;
            PD_MSG_FIELD(edt.guid) = base->guid;
            PD_MSG_FIELD(edt.metaDataPtr) = base;
            PD_MSG_FIELD(properties) = 0; // Not a runtime free since it was acquired using DB create
            if(pd->processMessage(pd, &msg, false)) {
                DPRINTF(DEBUG_LVL_WARN, "EDT (GUID: 0x%lx) could not release dynamically acquired DB (GUID: 0x%lx)\n",
                        base->guid, PD_MSG_FIELD(guid.guid));
                break;
            }
#undef PD_MSG
#undef PD_TYPE
            --count;
            ++extraToFree;
        }
        pd->pdFree(pd, derived->unkDbs);
    }

    // Now deal with the output event
    if(base->outputEvent != NULL_GUID) {
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DEP_SATISFY
        msg.type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
        PD_MSG_FIELD(guid.guid) = base->outputEvent;
        PD_MSG_FIELD(guid.metaDataPtr) = NULL;
        PD_MSG_FIELD(payload.guid) = retGuid;
        PD_MSG_FIELD(payload.metaDataPtr) = NULL;
        PD_MSG_FIELD(slot) = 0; // Always satisfy on slot 0. This will trickle to
                                // the finish latch if needed
        PD_MSG_FIELD(properties) = 0;
        // Ignore failure for now
        // FIXME: Probably need to be a bit more selective
        pd->processMessage(pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
    }
    return 0;
}

void destructTaskFactoryHc(ocrTaskFactory_t* base) {
    runtimeChunkFree((u64)base, NULL);
}

ocrTaskFactory_t * newTaskFactoryHc(ocrParamList_t* perInstance, u32 factoryId) {
    ocrTaskFactory_t* base = (ocrTaskFactory_t*)runtimeChunkAlloc(sizeof(ocrTaskFactoryHc_t), NULL);
    
    base->instantiate = FUNC_ADDR(ocrTask_t* (*) (ocrTaskFactory_t*, ocrFatGuid_t, u32, u64*, u32, u32, ocrFatGuid_t, ocrFatGuid_t*, ocrParamList_t*), newTaskHc);
    base->destruct =  FUNC_ADDR(void (*) (ocrTaskFactory_t*), destructTaskFactoryHc);
    base->factoryId = factoryId;
    
    base->fcts.destruct = FUNC_ADDR(u8 (*)(ocrTask_t*), destructTaskHc);
    base->fcts.satisfy = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t, u32), satisfyTaskHc);
    base->fcts.registerSignaler = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t, u32, bool), registerSignalerTaskHc);
    base->fcts.unregisterSignaler = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t, u32, bool), unregisterSignalerTaskHc);
    base->fcts.notifyDbAcquire = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t), notifyDbAcquireTaskHc);
    base->fcts.notifyDbRelease = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t), notifyDbReleaseTaskHc);
    base->fcts.execute = FUNC_ADDR(u8 (*)(ocrTask_t*), taskExecute);
    
    return base;
}

#endif /* ENABLE_TASK_HC */

#endif /* ENABLE_TASK_HC || ENABLE_TASKTEMPLATE_HC */
