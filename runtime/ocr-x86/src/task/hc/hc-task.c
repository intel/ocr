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

#ifdef OCR_ENABLE_EDT_PROFILING
extern struct _profileStruct gProfilingTable[] __attribute__((weak));
extern struct _dbWeightStruct gDbWeights[] __attribute__((weak));
#endif

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#ifdef OCR_ENABLE_PROFILING_STATISTICS
#endif
#endif /* OCR_ENABLE_STATISTICS */

#include "utils/profiler/profiler.h"

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
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));
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

    RESULT_PROPAGATE2(pd->fcts.processMessage(pd, &msg, true), NULL);

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
#ifdef OCR_ENABLE_EDT_PROFILING
    base->profileData = NULL;
    if(gProfilingTable) {
      int i;
      for(i = 0; ; i++) {
        if(gProfilingTable[i].fname == NULL) break;
            if(!ocrStrcmp((u8*)fctName, gProfilingTable[i].fname)) {
              base->profileData = &(gProfilingTable[i]);
              break;
            }
          }
    }

    base->dbWeights = NULL;
    if(gDbWeights) {
      int i;
      for(i = 0; ; i++) {
        if(gDbWeights[i].fname == NULL) break;
            if(!ocrStrcmp((u8*)fctName, gDbWeights[i].fname)) {
              base->dbWeights = &(gDbWeights[i]);
              break;
            }
          }
    }
#endif

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
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, msg, false));
#undef PD_TYPE
#define PD_TYPE PD_MSG_DEP_ADD
    msg->type = PD_MSG_DEP_ADD | PD_MSG_REQUEST;
    PD_MSG_FIELD(source) = sourceEvent;
    PD_MSG_FIELD(dest) = latchEvent;
    PD_MSG_FIELD(slot) = OCR_EVENT_LATCH_DECR_SLOT;
    PD_MSG_FIELD(properties) = DB_MODE_RO;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, msg, false));
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
    msg->type = PD_MSG_DEP_REGWAITER | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(waiter.guid) = self->base.guid;
    PD_MSG_FIELD(waiter.metaDataPtr) = self;
    PD_MSG_FIELD(dest.guid) = self->signalers[slot].guid;
    PD_MSG_FIELD(dest.metaDataPtr) = NULL;
    PD_MSG_FIELD(slot) = self->signalers[slot].slot;
    PD_MSG_FIELD(properties) = false; // not called from add-dependence
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, msg, true));
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


    task->frontierSlot = 0;
    task->slotSatisfiedCount = 0;
    task->lock = 0;
    task->unkDbs = NULL;
    task->countUnkDbs = 0;
    task->maxUnkDbs = 0;
    task->resolvedDeps = NULL;

    u32 i;
    for(i = 0; i < OCR_MAX_MULTI_SLOT; ++i) {
        task->doNotReleaseSlots[i] = 0ULL;
    }

    if(task->base.depc == 0) {
        task->signalers = END_OF_LIST;
    }
    // If we are creating a finish-edt
    if (hasProperty(properties, EDT_PROP_FINISH)) {
        ocrPolicyMsg_t msg;
        getCurrentEnv(NULL, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EVT_CREATE
        msg.type = PD_MSG_EVT_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD(guid.guid) = NULL_GUID;
        PD_MSG_FIELD(guid.metaDataPtr) = NULL;
        PD_MSG_FIELD(type) = OCR_EVENT_LATCH_T;
        PD_MSG_FIELD(properties) = 0;
        RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));

        ocrFatGuid_t latchFGuid = PD_MSG_FIELD(guid);
#undef PD_MSG
#undef PD_TYPE
        ASSERT(latchFGuid.guid != NULL_GUID && latchFGuid.metaDataPtr != NULL);

        if (parentLatch.guid != NULL_GUID) {
            DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on parent flatch 0x%lx\n", task->base.guid, parentLatch.guid);
            // Check in current finish latch
            getCurrentEnv(NULL, NULL, NULL, &msg);
            RESULT_PROPAGATE(finishLatchCheckin(pd, &msg, latchFGuid, parentLatch));
        }

        DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on self flatch 0x%lx\n", task->base.guid, latchFGuid.guid);
        // Check in the new finish scope
        // This will also link outputEvent to latchFGuid
        getCurrentEnv(NULL, NULL, NULL, &msg);
        RESULT_PROPAGATE(finishLatchCheckin(pd, &msg, outputEvent, latchFGuid));
        // Set edt's ELS to the new latch
        task->base.finishLatch = latchFGuid.guid;
    } else {
        // If the currently executing edt is in a finish scope,
        // but is not a finish-edt itself, just register to the scope
        if(parentLatch.guid != NULL_GUID) {
            DPRINTF(DEBUG_LVL_INFO, "Checkin 0x%lx on current flatch 0x%lx\n", task->base.guid, parentLatch.guid);
            // Check in current finish latch
            ocrPolicyMsg_t msg;
            getCurrentEnv(NULL, NULL, NULL, &msg);
            RESULT_PROPAGATE(finishLatchCheckin(pd, &msg, outputEvent, parentLatch));
        }
    }
    return 0;
}

/**
 * @brief sorted an array of regNode_t according to their GUID
 * Warning. 'notifyDbReleaseTaskHc' relies on this sort to be stable !
 */
 static void sortRegNode(regNode_t * array, u32 length) {
     if (length >= 2) {
        int idx;
        int sorted = 0;
        do {
            idx = sorted;
            regNode_t val = array[sorted+1];
            while((idx > -1) && (array[idx].guid > val.guid)) {
                idx--;
            }
            if (idx < sorted) {
                // shift by one to insert the element
                hal_memMove(&array[idx+2], &array[idx+1], sizeof(regNode_t)*(sorted-idx), false);
                array[idx+1] = val;
            }
            sorted++;
        } while (sorted < (length-1));
    }
}

/**
 * @brief Advance the DB iteration frontier to the next DB
 * This implementation iterates on the GUID-sorted signaler vector
 * Returns false when the end of depv is reached
 */
static u8 iterateDbFrontier(ocrTask_t *self) {
    ocrTaskHc_t * rself = ((ocrTaskHc_t *) self);
    regNode_t * depv = rself->signalers;
    u32 i = rself->frontierSlot;
    for (; i < self->depc; ++i) {
        // Important to do this before we call processMessage
        // because of the assert checks done in satisfyTaskHc
        rself->frontierSlot++;
        if (depv[i].guid != NULL_GUID) {
            // Because the frontier is sorted, we can check for duplicates here
            // and remember them to avoid double release
            if ((i > 0) && (depv[i-1].guid == depv[i].guid)) {
                rself->resolvedDeps[depv[i].slot].ptr = rself->resolvedDeps[depv[i-1].slot].ptr;
                ASSERT(depv[i].slot / 64 < OCR_MAX_MULTI_SLOT);
                rself->doNotReleaseSlots[depv[i].slot / 64] |= (1ULL << (depv[i].slot % 64));
            } else {
                // Issue registration request
                ocrPolicyDomain_t * pd = NULL;
                ocrPolicyMsg_t msg;
                getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
                msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
                PD_MSG_FIELD(guid.guid) = depv[i].guid; // DB guid
                PD_MSG_FIELD(guid.metaDataPtr) = NULL;
                PD_MSG_FIELD(edt.guid) = self->guid; // EDT guid
                PD_MSG_FIELD(edt.metaDataPtr) = self;
                PD_MSG_FIELD(ptr) = NULL;
                PD_MSG_FIELD(size) = 0; // avoids auto-serialization on outgoing
                PD_MSG_FIELD(edtSlot) = self->depc + 1; // RT slot
                PD_MSG_FIELD(properties) = depv[i].mode | DB_PROP_RT_ACQUIRE;
                u8 returnCode = pd->fcts.processMessage(pd, &msg, false);
                // DB_ACQUIRE is potentially asynchronous, check completion.
                // In shmem and dist HC PD, ACQUIRE is two-way, processed asynchronously
                // (the false in 'processMessage'). For now the CE/XE PD do not support this
                // mode so we need to check for the returnDetail of the acquire message instead.
                if ((returnCode == OCR_EPEND) || (PD_MSG_FIELD(returnDetail) == OCR_EBUSY)) {
                    return true;
                }
                // else, acquire took place and was successful, continue iterating
                ASSERT(msg.type & PD_MSG_RESPONSE); // 2x check
                rself->resolvedDeps[depv[i].slot].ptr = PD_MSG_FIELD(ptr);
#undef PD_MSG
#undef PD_TYPE
            }
        }
    }
    return false;
}

/**
 * @brief Give the task to the scheduler
 * Warning: The caller must ensure all dependencies have been satisfied
 * Note: static function only meant to factorize code.
 */
static u8 scheduleTask(ocrTask_t *self) {
    DPRINTF(DEBUG_LVL_INFO, "Schedule 0x%lx\n", self->guid);
    self->state = ALLACQ_EDTSTATE;
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
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

/**
 * @brief Dependences of the tasks have been satisfied
 * Warning: The caller must ensure all dependencies have been satisfied
 * Note: static function only meant to factorize code.
 */
static u8 taskAllDepvSatisfied(ocrTask_t *self) {
    //TODO DBX It feels the responsibility of fetching DB-related things should
    //be in the scheduler, however it is difficult to interact with the task
    //inners from there.
    DPRINTF(DEBUG_LVL_INFO, "All dependences satisfied for task 0x%lx\n", self->guid);
    // Now check if there's anything to do before scheduling
    // In this implementation we want to acquire locks for DBs in EW mode
    ocrTaskHc_t * rself = (ocrTaskHc_t *) self;
    rself->slotSatisfiedCount++; // Mark the slotSatisfiedCount as being all satisfied
    if (self->depc > 0) {
        ocrPolicyDomain_t * pd = NULL;
        getCurrentEnv(&pd, NULL, NULL, NULL);
        // Initialize the dependence list to be transmitted to the EDT's user code.
        u32 depc = self->depc;
        ocrEdtDep_t * resolvedDeps = pd->fcts.pdMalloc(pd, sizeof(ocrEdtDep_t)* depc);
        rself->resolvedDeps = resolvedDeps;
        regNode_t * signalers = rself->signalers;
        u32 i = 0;
        while(i < depc) {
            rself->signalers[i].slot = i; // reset the slot info
            resolvedDeps[i].guid = signalers[i].guid; // DB guids by now
            resolvedDeps[i].ptr = NULL; // resolved by acquire messages
            i++;
        }
        // Sort regnode in guid's ascending order.
        // This is the order in which we acquire the DBs
        sortRegNode(signalers, self->depc);
        // Start the DB acquisition process
        rself->frontierSlot = 0;
    }

    if (!iterateDbFrontier(self)) {
        scheduleTask(self);
    }
    return 0;
}

/******************************************************/
/* OCR-HC Task Implementation                         */
/******************************************************/


// Special sentinel values used to mark slots state

// A slot that contained an event has been satisfied
#define SLOT_SATISFIED_EVT              ((u32) -1)
// An ephemeral event has been registered on the slot
#define SLOT_REGISTERED_EPHEMERAL_EVT   ((u32) -2)
// A slot has been satisfied with a DB
#define SLOT_SATISFIED_DB               ((u32) -3)

u8 destructTaskHc(ocrTask_t* base) {
    DO_DEBUG_TYPE(TASK, DEBUG_LVL_INFO)
    PRINTF("EDT(INFO) [PD:0x0 W:0x0 EDT:0x0] Destroy 0x%lx\n", base->guid);
    END_DEBUG
    // FIXME: The above hack was added because the below causes a coredump. [Trac #219]
    //DPRINTF(DEBUG_LVL_INFO, "Destroy 0x%lx\n", base->guid);
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
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

u8 newTaskHc(ocrTaskFactory_t* factory, ocrFatGuid_t * edtGuid, ocrFatGuid_t edtTemplate,
                      u32 paramc, u64* paramv, u32 depc, u32 properties,
                      ocrFatGuid_t affinity, ocrFatGuid_t * outputEventPtr,
                      ocrTask_t *curEdt, ocrFatGuid_t parentLatch,
                      ocrParamList_t *perInstance) {

    // Get the current environment
    ocrPolicyDomain_t *pd = NULL;
    u32 i;
    getCurrentEnv(&pd, NULL, NULL, NULL);

    ocrFatGuid_t outputEvent = {.guid = NULL_GUID, .metaDataPtr = NULL};
    // We need an output event for the EDT if either:
    //  - the user requested one (outputEventPtr is non NULL)
    //  - the EDT is a finish EDT (and therefore we need to link
    //    the output event to the latch event)
    //  - the EDT is within a finish scope (and we need to link to
    //    that latch event)
    if (outputEventPtr != NULL || hasProperty(properties, EDT_PROP_FINISH) ||
            parentLatch.guid != NULL_GUID) {
        ocrPolicyMsg_t msg;
        getCurrentEnv(NULL, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EVT_CREATE
        msg.type = PD_MSG_EVT_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD(guid.guid) = NULL_GUID;
        PD_MSG_FIELD(guid.metaDataPtr) = NULL;
        PD_MSG_FIELD(properties) = 0;
        PD_MSG_FIELD(type) = OCR_EVENT_ONCE_T; // Output events of EDTs are non sticky

        RESULT_PROPAGATE2(pd->fcts.processMessage(pd, &msg, true), 1);
        outputEvent = PD_MSG_FIELD(guid);

#undef PD_MSG
#undef PD_TYPE
    }

    ocrPolicyMsg_t msg;
    // Create the task itself by getting a GUID
    getCurrentEnv(NULL, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_CREATE
    msg.type = PD_MSG_GUID_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = NULL_GUID;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    // We allocate everything in the meta-data to keep things simple
    PD_MSG_FIELD(size) = sizeof(ocrTaskHc_t) + paramc*sizeof(u64) + depc*sizeof(regNode_t);
    PD_MSG_FIELD(kind) = OCR_GUID_EDT;
    PD_MSG_FIELD(properties) = 0;
    RESULT_PROPAGATE2(pd->fcts.processMessage(pd, &msg, true), 1);
    ocrTaskHc_t *edt = (ocrTaskHc_t*)PD_MSG_FIELD(guid.metaDataPtr);
    ocrTask_t *base = (ocrTask_t*)edt;
    ASSERT(edt);

    // Set-up base structures
    base->guid = PD_MSG_FIELD(guid.guid);
    base->templateGuid = edtTemplate.guid;
    ASSERT(edtTemplate.metaDataPtr); // For now we just assume it is passed whole
    base->funcPtr = ((ocrTaskTemplate_t*)(edtTemplate.metaDataPtr))->executePtr;
    base->paramv = (paramc > 0) ? ((u64*)((u64)base + sizeof(ocrTaskHc_t))) : NULL;
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
        edt->signalers[i].mode = -1; //Systematically set when adding dependence
    }

    // Set up HC specific stuff
    RESULT_PROPAGATE2(initTaskHcInternal(edt, pd, curEdt, outputEvent, parentLatch, properties), 1);

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

    edtGuid->guid = base->guid;
    edtGuid->metaDataPtr = base;

    // Check to see if the EDT can be run
    if(base->depc == edt->slotSatisfiedCount) {
        DPRINTF(DEBUG_LVL_VVERB, "Scheduling task 0x%lx due to initial satisfactions\n",
                base->guid);
        RESULT_PROPAGATE2(taskAllDepvSatisfied(base), 1);
    }
    return 0;
}

u8 dependenceResolvedTaskHc(ocrTask_t * self, ocrGuid_t dbGuid, void * localDbPtr, u32 slot) {
    ocrTaskHc_t * rself = (ocrTaskHc_t *) self;
    // EDT already has all its dependences satisfied, now we're getting acquire notifications
    // should only happen on RT event slot to manage DB acquire
    ASSERT(slot == (self->depc+1));
    ASSERT(rself->slotSatisfiedCount == slot);
    // Implementation acquires DB sequentially, so the DB's GUID
    // must match the frontier's DB and we do not need to lock this code
    ASSERT(dbGuid == rself->signalers[rself->frontierSlot-1].guid);
    rself->resolvedDeps[rself->signalers[rself->frontierSlot-1].slot].ptr = localDbPtr;
    if (!iterateDbFrontier(self)) {
        scheduleTask(self);
    }
    return 0;
}

u8 satisfyTaskHc(ocrTask_t * base, ocrFatGuid_t data, u32 slot) {
    // An EDT has a list of signalers, but only registers
    // incrementally as signals arrive AND on non-persistent
    // events (latch or ONCE)
    // Assumption: signal frontier is initialized at slot zero
    // Whenever we receive a signal:
    //  - it can be from the frontier (we registered on it)
    //  - it can be a ONCE event
    //  - it can be a data-block being added (causing an immediate satisfy)

    ocrTaskHc_t * self = (ocrTaskHc_t *) base;

    // Replace the signaler's guid by the data guid, this is to avoid
    // further references to the event's guid, which is good in general
    // and crucial for once-event since they are being destroyed on satisfy.

    hal_lock32(&(self->lock));

    DPRINTF(DEBUG_LVL_VERB, "Satisfy on task 0x%lx slot %d with 0x%lx slotSatisfiedCount=%u frontierSlot=%u depc=%u\n",
        self->base.guid, slot, data.guid, self->slotSatisfiedCount, self->frontierSlot, base->depc);

    // Check to see if not already satisfied
    ASSERT_BLOCK_BEGIN(self->signalers[slot].slot != SLOT_SATISFIED_EVT)
    ocrTask_t * taskPut = NULL;
    getCurrentEnv(NULL, NULL, &taskPut, NULL);
    DPRINTF(DEBUG_LVL_WARN, "detected double satisfy on sticky for task 0x%lx on slot %d by 0x%lx\n", base->guid, slot, taskPut->guid);
    ASSERT_BLOCK_END
    ASSERT(self->slotSatisfiedCount < base->depc);

    self->slotSatisfiedCount++;
    self->signalers[slot].guid = data.guid;

    if(self->slotSatisfiedCount == base->depc) {
        DPRINTF(DEBUG_LVL_VERB, "Scheduling task 0x%lx, satisfied dependences %d/%d\n",
                self->base.guid, self->slotSatisfiedCount , base->depc);
        hal_unlock32(&(self->lock));
        // All dependences have been satisfied, schedule the edt
        RESULT_PROPAGATE(taskAllDepvSatisfied(base));
    } else {
        // Decide to keep both SLOT_SATISFIED_DB and SLOT_SATISFIED_EVT to be able to
        // disambiguate between events and db satisfaction. Not strictly necessary but
        // nice to have for debug.
        if (self->signalers[slot].slot != SLOT_SATISFIED_DB) {
            self->signalers[slot].slot = SLOT_SATISFIED_EVT;
        }
        // Check frontier status
        if (slot == self->frontierSlot) { // we are on the frontier slot
            // Try to advance the frontier over all consecutive satisfied events
            // and DB dependence that may be in flight (safe because we have the lock)
            u32 fsSlot = 0;
            do {
                self->frontierSlot++;
                DPRINTF(DEBUG_LVL_VERB, "Slot Increment on task 0x%lx slot %d with 0x%lx slotCount=%u slotFrontier=%u depc=%u\n",
                    self->base.guid, slot, data.guid, self->slotSatisfiedCount, self->frontierSlot, base->depc);
                fsSlot = self->signalers[self->frontierSlot].slot;
            } while((fsSlot == SLOT_SATISFIED_EVT) || (fsSlot == SLOT_SATISFIED_DB));
            // If here, there must be at least one satisfy that hasn't happened yet.
            ASSERT(self->slotSatisfiedCount < base->depc);

            // If we reach the end of the dependences, there's most likely a DB satisfy
            // in flight. Its .slot has been set, which is why we skipped over its slot
            // but the corresponding satisfy hasn't been executed yet. When it is,
            // slotSatisfiedCount equals depc and the task is scheduled.
            if (self->frontierSlot < base->depc) {
                // The slot we found is either:
                // 1- not known: addDependence hasn't occured yet (UNINITIALIZED_GUID)
                // 2- known: but the edt hasn't registered on it yet
                // 3- a once event not yet satisfied: (.slot == SLOT_REGISTERED_EPHEMERAL_EVT, registered but not yet satisfied)
                if ((self->signalers[self->frontierSlot].guid != UNINITIALIZED_GUID) &&
                    (self->signalers[self->frontierSlot].slot != SLOT_REGISTERED_EPHEMERAL_EVT)) {
                    // Just for debugging purpose
                    ocrFatGuid_t signalerGuid;
                    signalerGuid.guid = self->signalers[self->frontierSlot].guid;
                    // Warning double check if that works for regular implementation
                    signalerGuid.metaDataPtr = NULL; // should be ok because guid encodes the kind in distributed
                    ocrGuidKind signalerKind = OCR_GUID_NONE;
                    ocrPolicyDomain_t *pd = NULL;
                    ocrPolicyMsg_t msg;
                    getCurrentEnv(&pd, NULL, NULL, &msg);
                    deguidify(pd, &signalerGuid, &signalerKind);
                    ASSERT((signalerKind == OCR_GUID_EVENT_STICKY) || (signalerKind == OCR_GUID_EVENT_IDEM));
                    hal_unlock32(&(self->lock));
                    // Case 2: A sticky, the EDT registers as a lazy waiter
                    // Here it should be ok to read the frontierSlot since we are on the frontier
                    // only a satisfy on the event in that slot can advance the frontier and we
                    // haven't registered on it yet.
                    u8 res = registerOnFrontier(self, pd, &msg, self->frontierSlot);
                    return res;
                }
                //else:
                // case 1, registerSignaler will do the registration
                // case 3, just have to wait for the satisfy on the once event to happen.
            }
        }
        //else: not on frontier slot, nothing to do
        // Two cases:
        // - The slot has just been marked as satisfied but the frontier
        //   hasn't reached that slot yet. Most likely the satisfy is on
        //   an ephemeral event or directly with a db. The frontier will
        //   eventually reach this slot at a later point.
        // - There's a race between 'register' setting the .slot to the DB guid
        //   and a concurrent satisfy incrementing the frontier. i.e. it skips
        //   over the DB guid because its .slot is 'SLOT_SATISFIED_DB'.
        //   When the DB satisfy happens it falls-through here.
        hal_unlock32(&(self->lock));
    }
    return 0;
}

/**
 * Can be invoked concurrently, however each invocation should be for a different slot
 */
u8 registerSignalerTaskHc(ocrTask_t * base, ocrFatGuid_t signalerGuid, u32 slot,
                            ocrDbAccessMode_t mode, bool isDepAdd) {
    ASSERT(isDepAdd); // This should only be called when adding a dependence

    ocrTaskHc_t * self = (ocrTaskHc_t *) base;
    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);
    ocrGuidKind signalerKind = OCR_GUID_NONE;
    deguidify(pd, &signalerGuid, &signalerKind);
    regNode_t * node = &(self->signalers[slot]);
    node->mode = mode;
    ASSERT(node->slot == slot); // assumption from initialization
    ASSERT(signalerGuid.guid != NULL_GUID); // This should have been caught earlier on
    hal_lock32(&(self->lock));
    node->guid = signalerGuid.guid;
    //DIST-TODO this is the reason why we need to introduce new kinds of guid
    //because we don't have support for cloning metadata around yet
    if(signalerKind & OCR_GUID_EVENT) {
        if((signalerKind == OCR_GUID_EVENT_ONCE) ||
                (signalerKind == OCR_GUID_EVENT_LATCH)) {
            node->slot = SLOT_REGISTERED_EPHEMERAL_EVT; // To record this slot is for a once event
            hal_unlock32(&(self->lock));
        } else {
            // Must be a sticky event. Read the frontierSlot now that we have the lock.
            // If 'register' is on the frontier, do the registration. Otherwise the edt
            // will lazily register on the signalerGuid when the frontier reaches the
            // signaler's slot.
            bool doRegister = (slot == self->frontierSlot);
            hal_unlock32(&(self->lock));
            if(doRegister) {
                // The EDT registers itself as a waiter here
                ocrPolicyDomain_t *pd = NULL;
                ocrPolicyMsg_t msg;
                getCurrentEnv(&pd, NULL, NULL, &msg);
                RESULT_PROPAGATE(registerOnFrontier(self, pd, &msg, slot));
            }
        }
    } else {
        ASSERT(signalerKind == OCR_GUID_DB);
        // Here we could use SLOT_SATISFIED_EVT directly, but if we do,
        // when satisfy is called we won't be able to figure out if the
        // value was set for a DB here, or by a previous satisfy.
        node->slot = SLOT_SATISFIED_DB;
        // Setting the slot and incrementing the frontier in two steps
        // introduce a race between the satisfy here after and another
        // concurrent satisfy advancing the frontier.
        hal_unlock32(&(self->lock));
        //Convert to a satisfy now that we've recorded the mode
        //NOTE: Could improve implementation by figuring out how
        //to properly iterate the frontier when adding the DB
        //potentially concurrently with satifies.
        ocrPolicyMsg_t registerMsg;
        getCurrentEnv(NULL, NULL, NULL, &registerMsg);
    #define PD_MSG (&registerMsg)
    #define PD_TYPE PD_MSG_DEP_SATISFY
        registerMsg.type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
        PD_MSG_FIELD(guid.guid) = base->guid;
        PD_MSG_FIELD(guid.metaDataPtr) = NULL;
        PD_MSG_FIELD(payload.guid) = signalerGuid.guid;
        PD_MSG_FIELD(payload.metaDataPtr) = NULL;
        PD_MSG_FIELD(slot) = slot;
        PD_MSG_FIELD(properties) = 0;
        RESULT_PROPAGATE(pd->fcts.processMessage(pd, &registerMsg, true));
    #undef PD_MSG
    #undef PD_TYPE
    }

    DPRINTF(DEBUG_LVL_INFO, "AddDependence from 0x%lx to 0x%lx slot %d\n",
        signalerGuid.guid, base->guid, slot);
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
    if(derived->maxUnkDbs == 0) {
        derived->unkDbs = (ocrGuid_t*)pd->fcts.pdMalloc(pd, sizeof(ocrGuid_t)*8);
        derived->maxUnkDbs = 8;
    } else {
        if(derived->maxUnkDbs == derived->countUnkDbs) {
            ocrGuid_t *oldPtr = derived->unkDbs;
            derived->unkDbs = (ocrGuid_t*)pd->fcts.pdMalloc(pd, sizeof(ocrGuid_t)*derived->maxUnkDbs*2);
            ASSERT(derived->unkDbs);
            hal_memCopy(derived->unkDbs, oldPtr, sizeof(ocrGuid_t)*derived->maxUnkDbs, false);
            pd->fcts.pdFree(pd, oldPtr);
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
    if ((derived->unkDbs != NULL) || (base->depc != 0)) {
        // Search in the list of DBs created by the EDT
        u64 maxCount = derived->countUnkDbs;
        u64 count = 0;
        DPRINTF(DEBUG_LVL_VERB, "Notifying EDT (GUID: 0x%lx) that it released db (GUID: 0x%lx)\n",
                base->guid, db.guid);
        while(count < maxCount) {
            // We bound our search (in case there is an error)
            if(db.guid == derived->unkDbs[count]) {
                DPRINTF(DEBUG_LVL_VVERB, "Dynamic Releasing DB @ 0x%lx (GUID 0x%lx) from EDT 0x%lx, match in unkDbs list for count %lu\n",
                       db.guid, base->guid, count);
                // printf("Dynamic Releasing DB (GUID 0x%lx) from EDT 0x%lx, match in unkDbs list for count %lu\n",
                //        db.guid, base->guid, count);
                derived->unkDbs[count] = derived->unkDbs[maxCount - 1];
                --(derived->countUnkDbs);
                return 0;
            }
            ++count;
        }

        // Search DBs in dependences
        maxCount = base->depc;
        count = 0;
        while(count < maxCount) {
            // We bound our search (in case there is an error)
            if(db.guid == derived->resolvedDeps[count].guid) {
                DPRINTF(DEBUG_LVL_VVERB, "Dynamic Releasing DB (GUID 0x%lx) from EDT 0x%lx, "
                        "match in dependence list for count %lu\n",
                        db.guid, base->guid, count);
                ASSERT(count / 64 < OCR_MAX_MULTI_SLOT);
                derived->doNotReleaseSlots[count / 64] |= (1ULL << (count % 64));
                // we can return on the first instance found since iterateDbFrontier
                // already marked duplicated DB and the selection sort in sortRegNode is stable.
                return 0;
            }
            ++count;
        }
    }
    // not found means it's an error or it has already been released
    return 0;
}

u8 taskExecute(ocrTask_t* base) {
    DPRINTF(DEBUG_LVL_INFO, "Execute 0x%lx paramc:%d depc:%d\n", base->guid, base->paramc, base->depc);
    ocrTaskHc_t* derived = (ocrTaskHc_t*)base;
    // In this implementation each time a signaler has been satisfied, its guid
    // has been replaced by the db guid it has been satisfied with.
    u32 paramc = base->paramc;
    u64 * paramv = base->paramv;
    u32 depc = base->depc;
    ocrPolicyDomain_t *pd = NULL;
    ocrEdtDep_t * depv = derived->resolvedDeps;
    ocrPolicyMsg_t msg;
    ASSERT(derived->unkDbs == NULL); // Should be no dynamically acquired DBs before running
    getCurrentEnv(&pd, NULL, NULL, NULL);

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
    {

#ifdef OCR_ENABLE_VISUALIZER
        u64 startTime = getTimeNs();
#endif

        START_PROFILE(userCode);
        retGuid = base->funcPtr(paramc, paramv, depc, depv);
        EXIT_PROFILE;

#ifdef OCR_ENABLE_VISUALIZER
        u64 endTime = getTimeNs();
        DPRINTF(DEBUG_LVL_INFO, "Execute 0x%lx FctName: %s Start: %lu End: %lu\n", base->guid, base->name, startTime, endTime);
#endif
    }

#ifdef OCR_ENABLE_STATISTICS
    // We now say that the worker is done executing the EDT
    statsEDT_END(pd, ctx->sourceObj, curWorker, base->guid, base);
#endif /* OCR_ENABLE_STATISTICS */
    DPRINTF(DEBUG_LVL_INFO, "End_Execution 0x%lx\n", base->guid);

    // edt user code is done, if any deps, release data-blocks
    if(depc != 0) {
        START_PROFILE(ta_hc_dbRel);
        u32 i;
        for(i=0; i < depc; ++i) {
            u32 j = i / 64;
            if ((depv[i].guid != NULL_GUID) &&
               ((j >= OCR_MAX_MULTI_SLOT) || (derived->doNotReleaseSlots[j] == 0) ||
                ((j < OCR_MAX_MULTI_SLOT) && (((1ULL << (i % 64)) & derived->doNotReleaseSlots[j]) == 0)))) {
                getCurrentEnv(NULL, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_RELEASE
                msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
                PD_MSG_FIELD(guid.guid) = depv[i].guid;
                PD_MSG_FIELD(guid.metaDataPtr) = NULL;
                PD_MSG_FIELD(edt.guid) = base->guid;
                PD_MSG_FIELD(edt.metaDataPtr) = base;
                PD_MSG_FIELD(ptr) = NULL;
                PD_MSG_FIELD(size) = 0; // TODO check that's set properly for release by hc-dist-policy.c
                PD_MSG_FIELD(properties) = DB_PROP_RT_ACQUIRE; // Runtime release
                // Ignore failures at this point
                pd->fcts.processMessage(pd, &msg, true);
#undef PD_MSG
#undef PD_TYPE
            }
        }
        pd->fcts.pdFree(pd, depv);
        EXIT_PROFILE;
    }

    // We now release all other data-blocks that we may potentially
    // have acquired along the way
    if(derived->unkDbs != NULL) {
        ocrGuid_t *extraToFree = derived->unkDbs;
        u64 count = derived->countUnkDbs;
        while(count) {
            getCurrentEnv(NULL, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_RELEASE
            msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
            PD_MSG_FIELD(guid.guid) = extraToFree[0];
            PD_MSG_FIELD(guid.metaDataPtr) = NULL;
            PD_MSG_FIELD(edt.guid) = base->guid;
            PD_MSG_FIELD(edt.metaDataPtr) = base;
            PD_MSG_FIELD(ptr) = NULL;
            PD_MSG_FIELD(size) = 0;
            PD_MSG_FIELD(properties) = 0; // Not a runtime free since it was acquired using DB create
            if(pd->fcts.processMessage(pd, &msg, true)) {
                DPRINTF(DEBUG_LVL_WARN, "EDT (GUID: 0x%lx) could not release dynamically acquired DB (GUID: 0x%lx)\n",
                        base->guid, PD_MSG_FIELD(guid.guid));
                break;
            }
#undef PD_MSG
#undef PD_TYPE
            --count;
            ++extraToFree;
        }
        pd->fcts.pdFree(pd, derived->unkDbs);
    }

    // Now deal with the output event
    if(base->outputEvent != NULL_GUID) {
        if(retGuid != NULL_GUID) {
            getCurrentEnv(NULL, NULL, NULL, &msg);
    #define PD_MSG (&msg)
    #define PD_TYPE PD_MSG_DEP_ADD
            msg.type = PD_MSG_DEP_ADD | PD_MSG_REQUEST;
            PD_MSG_FIELD(source.guid) = retGuid;
            PD_MSG_FIELD(dest.guid) = base->outputEvent;
            PD_MSG_FIELD(slot) = 0; // Always satisfy on slot 0. This will trickle to
            // the finish latch if needed
            PD_MSG_FIELD(properties) = DB_MODE_RO;
            // Ignore failure for now
            // FIXME: Probably need to be a bit more selective
            pd->fcts.processMessage(pd, &msg, false);
    #undef PD_MSG
    #undef PD_TYPE
        } else {
            getCurrentEnv(NULL, NULL, NULL, &msg);
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
            pd->fcts.processMessage(pd, &msg, false);
    #undef PD_MSG
    #undef PD_TYPE
        }
    }
    return 0;
}

void destructTaskFactoryHc(ocrTaskFactory_t* base) {
    runtimeChunkFree((u64)base, NULL);
}

ocrTaskFactory_t * newTaskFactoryHc(ocrParamList_t* perInstance, u32 factoryId) {
    ocrTaskFactory_t* base = (ocrTaskFactory_t*)runtimeChunkAlloc(sizeof(ocrTaskFactoryHc_t), NULL);

    base->instantiate = FUNC_ADDR(u8 (*) (ocrTaskFactory_t*, ocrFatGuid_t*, ocrFatGuid_t, u32, u64*, u32, u32, ocrFatGuid_t, ocrFatGuid_t*, ocrTask_t *, ocrFatGuid_t, ocrParamList_t*), newTaskHc);
    base->destruct =  FUNC_ADDR(void (*) (ocrTaskFactory_t*), destructTaskFactoryHc);
    base->factoryId = factoryId;

    base->fcts.destruct = FUNC_ADDR(u8 (*)(ocrTask_t*), destructTaskHc);
    base->fcts.satisfy = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t, u32), satisfyTaskHc);
    base->fcts.registerSignaler = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t, u32, ocrDbAccessMode_t, bool), registerSignalerTaskHc);
    base->fcts.unregisterSignaler = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t, u32, bool), unregisterSignalerTaskHc);
    base->fcts.notifyDbAcquire = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t), notifyDbAcquireTaskHc);
    base->fcts.notifyDbRelease = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrFatGuid_t), notifyDbReleaseTaskHc);
    base->fcts.execute = FUNC_ADDR(u8 (*)(ocrTask_t*), taskExecute);
    base->fcts.dependenceResolved = FUNC_ADDR(u8 (*)(ocrTask_t*, ocrGuid_t, void*, u32), dependenceResolvedTaskHc);
    return base;
}
#endif /* ENABLE_TASK_HC */

#endif /* ENABLE_TASK_HC || ENABLE_TASKTEMPLATE_HC */
