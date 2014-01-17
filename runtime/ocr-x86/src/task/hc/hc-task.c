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

void destructTaskTemplateHc(ocrTaskTemplate_t *self) {
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
    RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
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

    RESULT_ASSERT(pd->processMessage(pd, &msg, true), ==, 0);

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
    base->guid = UNINITIALIZED_GUID;
    
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
    base->fcts.destruct = FUNC_ADDR(void (*)(ocrTaskTemplate_t*), destructTaskTemplateHc);
    return base;
}

#endif /* ENABLE_TASKTEMPLATE_HC */

#ifdef ENABLE_TASK_HC

/******************************************************/
/* OCR HC latch utilities                             */
/******************************************************/

ocrFatGuid_t getFinishLatch(ocrTask_t * edt) {
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
static void finishLatchCheckin(ocrPolicyDomain_t *pd, ocrPolicyMsg_t *msg,
                               ocrFatGuid_t sourceEvent, ocrFatGuid_t latchEvent) {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DEP_SATISFY
    msg->type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid) = latchEvent;
    PD_MSG_FIELD(payload.guid) = NULL_GUID;
    PD_MSG_FIELD(payload.metaDataPtr) = NULL;
    PD_MSG_FIELD(slot) = OCR_EVENT_LATCH_INCR_SLOT;
    PD_MSG_FIELD(properties) = 0;
    RESULT_ASSERT(pd->processMessage(pd, msg, false), ==, 0);
#undef PD_TYPE
#define PD_TYPE PD_MSG_DEP_ADD
    msg->type = PD_MSG_DEP_ADD | PD_MSG_REQUEST;
    PD_MSG_FIELD(source) = sourceEvent;
    PD_MSG_FIELD(dest) = latchEvent;
    PD_MSG_FIELD(slot) = OCR_EVENT_LATCH_DECR_SLOT;
    PD_MSG_FIELD(properties) = 0; // TODO: do we want a mode for this?
    RESULT_ASSERT(pd->processMessage(pd, msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
}

// satisfies the decr slot of a finish latch event
// May not be needed
/*
static void finishLatchCheckout(ocrPolicyDomain_t *pd, ocrPolicyMsg_t *msg,
                                ocrFatGuid_t latchEvent) {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DEP_SATISFY
    msg->type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid) = latchEvent;
    PD_MSG_FIELD(payload.guid) = NULL_GUID;
    PD_MSG_FIELD(payload.metaDataPtr) = NULL;
    PD_MSG_FIELD(slot) = OCR_EVENT_LATCH_DECR_SLOT;
    PD_MSG_FIELD(properties) = 0;
    RESULT_ASSERT(pd->processMessage(pd, msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
}

// May not be needed
static bool isFinishLatchOwner(ocrEvent_t * finishLatch, ocrGuid_t edtGuid) {
    return (finishLatch != NULL) && (((ocrEventHcFinishLatch_t *)finishLatch)->ownerGuid == edtGuid);
}
*/

/******************************************************/
/* Random helper functions                            */
/******************************************************/

static inline bool hasProperty(u32 properties, u32 property) {
    return properties & property;
}

/******************************************************/
/* OCR-HC Support functions                           */
/******************************************************/

static void initTaskHcInternal(ocrTaskHc_t *task, ocrPolicyDomain_t * pd,
                               ocrTask_t *curTask, ocrFatGuid_t outputEvent,
                               ocrFatGuid_t parentLatch, u32 properties) {

    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);

    task->frontierSlot = 0;
    task->slotSatisfiedCount = 0;
    
    if(task->base.depc == 0) {
        task->signalers = END_OF_LIST;
    }
    // If we are creating a finish-edt
    if (hasProperty(properties, EDT_PROP_FINISH)) {
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EVT_CREATE
        PD_MSG_FIELD(guid.guid) = NULL_GUID;
        PD_MSG_FIELD(guid.metaDataPtr) = NULL;
        PD_MSG_FIELD(type) = OCR_EVENT_FINISH_LATCH_T;
        PD_MSG_FIELD(properties) = 0;
        RESULT_ASSERT(pd->processMessage(pd, &msg, true), ==, 0);

        ocrFatGuid_t latchFGuid = PD_MSG_FIELD(guid);
#undef PD_MSG
#undef PD_TYPE
        ASSERT(latchFGuid.guid != NULL_GUID && latchFGuid.metaDataPtr != NULL);
        
        ocrEventHcFinishLatch_t * hcLatch = (ocrEventHcFinishLatch_t *)latchFGuid.metaDataPtr;
        // Set the owner of the latch
        hcLatch->ownerGuid = task->base.guid;
        if (parentLatch.guid != NULL_GUID) {
            DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on parent flatch 0x%lx\n", task->base.guid, parentLatch.guid);
            // Check in current finish latch
            finishLatchCheckin(pd, &msg, latchFGuid, parentLatch);
        }
        
        DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on self flatch 0x%lx\n", task->base.guid, latchFGuid.guid);
        // Check in the new finish scope
        // This will also link outputEvent to latchFGuid
        finishLatchCheckin(pd, &msg, outputEvent, latchFGuid);
        // Set edt's ELS to the new latch
        task->base.finishLatch = latchFGuid.guid;
    } else {
        // If the currently executing edt is in a finish scope,
        // but is not a finish-edt itself, just register to the scope
        if(parentLatch.guid != NULL_GUID) {
            DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on current flatch 0x%lx\n", task->base.guid, parentLatch.guid);
            // Check in current finish latch
            finishLatchCheckin(pd, &msg, outputEvent, parentLatch);
        }
    }
}

/**
 * @brief Schedules a task.
 * Warning: The caller must ensure all dependencies have been satisfied
 * Note: static function only meant to factorize code.
 */
static void taskSchedule(ocrTask_t *self) {
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
    RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
}

/******************************************************/
/* OCR-HC Task Implementation                         */
/******************************************************/

void destructTaskHc(ocrTask_t* base) {
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
    RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
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
        PD_MSG_FIELD(type) = OCR_EVENT_STICKY_T;

        RESULT_ASSERT(pd->processMessage(pd, &msg, true), ==, 0);
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
    RESULT_ASSERT(pd->processMessage(pd, &msg, true), ==, 0);
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
    // Set up HC specific stuff
    initTaskHcInternal(edt, pd, curEdt, outputEvent, parentLatch, properties);

    // Set up outputEventPtr:
    //   - if a finish EDT, wait on its latch event
    //   - if not a finish EDT, wait on its output event
    if(base->finishLatch) {
        outputEventPtr->guid = base->finishLatch;
    } else {
        outputEventPtr->guid = base->outputEvent;
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
    DPRINTF(DEBUG_LVL_INFO, "Create 0x%lx depc %d outputEvent 0x%lx\n", base->guid, depc, outputEventPtr->guid);

    // Check to see if the EDT can be run
    if(base->depc == edt->slotSatisfiedCount) {
        taskSchedule(base);
    }
    return base;
}

u8 satisfyTaskHc(ocrTask_t * base, ocrFatGuid_t data, u32 slot) {
    // An EDT has a list of signalers, but only registers
    // incrementally as signals arrive.
    // Assumption: signal frontier is initialized at slot zero
    // Whenever we receive a signal, it can only be from the
    // current signal frontier, since it is the only signaler
    // the edt is registered with at that time.
    ocrTaskHc_t * self = (ocrTaskHc_t *) base;

    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);

    // TODO: REC: ONCE event seems to not work fully.
    // This may not be needed once fix in place
    // I think we need to register with ONCE events immediately
    // and handle things differently
    /*
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
    */
    
    // Replace the signaler's guid by the data guid, this to avoid
    // further references to the event's guid, which is good in general
    // and crucial for once-event since they are being destroyed on satisfy.
    ASSERT(self->signalers[slot].slot == slot ||
           self->signalers[slot].slot != (u32)-2); // Checks if not already satisfied
                                                   // -2 means once event or latch
    self->signalers[slot].guid = data.guid;
    self->signalers[slot].slot = (u32)-1; // Say that it is satisfied
    if(++self->slotSatisfiedCount == base->depc) {
        // All dependencies have been satisfied, schedule the edt
        taskSchedule(base);
    } else if(self->frontierSlot == slot) {
        // We need to go register to the next non-once dependence
        while(self->signalers[++self->frontierSlot].slot != slot) ;
        ASSERT(self->frontierSlot < base->depc);
        // We found a slot that is == to slot (so unsatisfied and not once
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DEP_ADD
        msg.type = PD_MSG_DEP_ADD | PD_MSG_REQUEST;
        PD_MSG_FIELD(source.guid) = self->signalers[self->frontierSlot].guid;
        PD_MSG_FIELD(source.metaDataPtr) = NULL;
        PD_MSG_FIELD(dest.guid) = base->guid;
        PD_MSG_FIELD(dest.metaDataPtr) = base;
        PD_MSG_FIELD(slot) = self->signalers[self->frontierSlot].slot;
        PD_MSG_FIELD(properties) = 0;
        RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
    }
    return 0;
}

u8 registerSignalerTaskHc(ocrTask_t * base, ocrFatGuid_t signalerGuid, u32 slot) {
    // Check if event or data-block
    ASSERT((signalerGuid.guid == NULL_GUID) || isEventGuid(signalerGuid.guid)
           || isDatablockGuid(signalerGuid.guid));
    ocrTaskHc_t * self = (ocrTaskHc_t *) base;
    regNode_t * node = &(self->signalers[slot]);
    
    node->guid = signalerGuid.guid;
    if(signalerGuid.guid == NULL_GUID) {
        node->slot = slot;
    } else if(isEventGuid(signalerGuid.guid)) {
        node->slot = slot;
        // REC: TODO: This is dangerous as it implies a deguidify which
        // may not be very easy to do. It is a read-only thing though
        // so maybe it is OK (change deguidify to specify mode?)
        if(isEventGuidOfKind(signalerGuid.guid, OCR_EVENT_ONCE_T)
           || isEventGuidOfKind(signalerGuid.guid, OCR_EVENT_LATCH_T)) {
            node->slot = (u32)-2; // To signal that this is a once event
        }
    } else if(isDatablockGuid(signalerGuid.guid)) {
        node->slot = (u32)-1; // Already satisfied
        ++(self->slotSatisfiedCount);
    } else {
        ASSERT(0);
    }
    if(base->depc == self->slotSatisfiedCount) {
        taskSchedule(base);
    }
    DPRINTF(DEBUG_LVL_INFO, "AddDependence from 0x%lx to 0x%lx slot %d\n", signalerGuid.guid, base->guid, slot);
    return 0;
}

u8 unregisterSignalerTaskHc(ocrTask_t * base, ocrFatGuid_t signalerGuid, u32 slot) {
    ASSERT(0); // We don't support this at this time...
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
    if (depc != 0) {
        u32 i = 0;
        //TODO would be nice to resolve regNode into guids before
        depv = pd->pdMalloc(pd, sizeof(ocrEdtDep_t)*depc);
        // Double-check we're not rescheduling an already executed edt
        ASSERT(derived->signalers != END_OF_LIST);
        while( i < depc ) {
            //TODO would be nice to standardize that on satisfy
            depv[i].guid = derived->signalers[i].guid;
            if(depv[i].guid != NULL_GUID) {
                ASSERT(isDatablockGuid(depv[i].guid));
                // We send a message that we want to acquire the DB
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
                msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
                PD_MSG_FIELD(guid.guid) = depv[i].guid;
                PD_MSG_FIELD(guid.metaDataPtr) = NULL;
                PD_MSG_FIELD(edt.guid) = base->guid;
                PD_MSG_FIELD(edt.metaDataPtr) = base;
                PD_MSG_FIELD(properties) = 1; // Runtime acquire
                RESULT_ASSERT(pd->processMessage(pd, &msg, true), ==, 0);
                depv[i].ptr = PD_MSG_FIELD(ptr);
#undef PD_MSG
#undef PD_TYPE
            } else {
                depv[i].ptr = NULL;
            }
            ++i;
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
    
    ocrGuid_t retGuid = base->funcPtr(paramc, paramv, depc, depv);
#ifdef OCR_ENABLE_STATISTICS
    // We now say that the worker is done executing the EDT
    statsEDT_END(pd, ctx->sourceObj, curWorker, base->guid, base);
#endif /* OCR_ENABLE_STATISTICS */

    // edt user code is done, if any deps, release data-blocks
    if(depc != 0) {
        u32 i;
        for(i=0; i < depc; ++i) {
            if(depv[i].guid != NULL_GUID) {
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_RELEASE
                msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST;
                PD_MSG_FIELD(guid.guid) = depv[i].guid;
                PD_MSG_FIELD(guid.metaDataPtr) = NULL;
                PD_MSG_FIELD(edt.guid) = base->guid;
                PD_MSG_FIELD(edt.metaDataPtr) = base;
                PD_MSG_FIELD(properties) = 1; // Runtime release
                RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
            }
        }
    }
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
        RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0);
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
    
    base->fcts.destruct = FUNC_ADDR(void (*)(ocrTask_t*), destructTaskHc);
    base->fcts.satisfy = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t, u32), satisfyTaskHc);
    base->fcts.registerSignaler = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t, u32), registerSignalerTaskHc);
    base->fcts.unregisterSignaler = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t, u32), unregisterSignalerTaskHc);
    base->fcts.execute = FUNC_ADDR(u8 (*)(ocrTask_t*), taskExecute);
    
    return base;
}

#endif /* ENABLE_TASK_HC */

#endif /* ENABLE_TASK_HC || ENABLE_TASKTEMPLATE_HC */
