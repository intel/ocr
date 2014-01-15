/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_WORKER_XE

#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime-types.h"
#include "ocr-sysboot.h"
#include "ocr-types.h"
#include "ocr-worker.h"
#include "worker/xe/xe-worker.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

#define DEBUG_TYPE WORKER

/******************************************************/
/* OCR-XE WORKER                                      */
/******************************************************/

/**
 * The computation worker routine that asks work to the scheduler
 */
static void * workerRoutine(launchArg_t * arg);

// Convenient to have an id to index workers in pools
static inline u64 getWorkerId(ocrWorker_t * worker) {
    ocrWorkerXe_t * xeWorker = (ocrWorkerXe_t *) worker;
    return xeWorker->id;
}

static void workerLoop(ocrWorker_t * worker) {
    ocrPolicyDomain_t *pd = worker->pd;
    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);
    while(worker->fcts.isRunning(worker)) {
        ocrFatGuid_t taskGuid = {.guid = NULL_GUID, .metaDataPtr = NULL};
        u32 count = 1;
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_COMM_TAKE
        msg.srcLocation = pd->myLocation;
        msg.type = PD_MSG_COMM_TAKE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE; 
        PD_MSG_FIELD(guids) = &taskGuid;
        PD_MSG_FIELD(guidCount) = count;
        PD_MSG_FIELD(properties) = 0;
        PD_MSG_FIELD(type) = OCR_GUID_EDT;
        if(pd->processMessage(pd, &msg, true) == 0) {
            // We got a response
            count = PD_MSG_FIELD(guidCount);
			ASSERT(count > 0);
            if(count == 1) {
                // REC: TODO: Do we need a message to execute this
                ASSERT(taskGuid.guid != NULL_GUID && taskGuid.metaDataPtr != NULL);
                worker->curTask = (ocrTask_t*)taskGuid.metaDataPtr;
                worker->curTask->fctPtrs->execute = PD_MSG_FIELD(extra); // task execute func ptr is set in the "extra" field of msg
                worker->curTask->fctPtrs->execute(worker->curTask); 
                worker->curTask = NULL;
                // Destroy the work
#undef PD_MSG
#undef PD_TYPE
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_WORK_DESTROY
                msg.type = PD_MSG_WORK_DESTROY | PD_MSG_REQUEST;
                PD_MSG_FIELD(guid) = taskGuid;
                PD_MSG_FIELD(properties) = 0;
                RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
            } else {
        		// TODO: In the future, potentially take more than one)
				ASSERT(0);
			}
        }
    } /* End of while loop */
}

static void * workerRoutine(launchArg_t* launchArg) {
    // Need to pass down a data-structure
    ocrWorker_t * worker = (ocrWorker_t *) launchArg->arg;
    ocrPolicyDomain_t *pd = worker->pd;
    pd->pdFree(pd, launchArg); // Free the launch argument
    
    // associate current thread with the worker
    //associate_comp_platform_and_worker(pd, worker);
    
    DPRINTF(DEBUG_LVL_INFO, "Starting scheduler routine of worker %ld\n", getWorkerId(worker));
    workerLoop(worker);
    return NULL;
}

void destructWorkerXe(ocrWorker_t * base) {
    u64 i = 0;
    while(i < base->computeCount) {
        base->computes[i]->fcts.destruct(base->computes[i]);
        ++i;
    }
    runtimeChunkFree((u64)(base->computes), NULL);    
    runtimeChunkFree((u64)base, NULL);
}

/**
 * Builds an instance of a XE worker
 */
ocrWorker_t* newWorkerXe (ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    ocrWorkerXe_t * worker = (ocrWorkerXe_t*)runtimeChunkAlloc(
        sizeof(ocrWorkerXe_t), NULL);
    ocrWorker_t * base = (ocrWorker_t *) worker;

    base->fguid.guid = UNINITIALIZED_GUID;
    base->fguid.metaDataPtr = base;
    base->pd = NULL;
    base->curTask = NULL;
    base->fcts = factory->workerFcts;
    base->type = SLAVE_WORKERTYPE;
    
    worker->id = ((paramListWorkerXeInst_t*)perInstance)->workerId;
    worker->run = false;

    return base;
}

void xeStartWorker(ocrWorker_t * base, ocrPolicyDomain_t * policy) {
    // Get a GUID
    guidify(policy, (u64)base, &(base->fguid), OCR_GUID_WORKER);
    base->pd = policy;
    
    ocrWorkerXe_t * xeWorker = (ocrWorkerXe_t *) base;
    xeWorker->run = true;

    // Starts everybody, the first comp-platform has specific
    // code to represent the master thread.
    u64 computeCount = base->computeCount;
    // What the compute target will execute
    launchArg_t * launchArg = policy->pdMalloc(policy, sizeof(launchArg_t));
    launchArg->routine = workerRoutine;
    launchArg->arg = base;
    ASSERT(computeCount == 1); // For now; o/w have to create more launchArg
    u64 i = 0;
    for(i = 0; i < computeCount; i++) {
        base->computes[i]->fcts.start(base->computes[i], policy, launchArg);
#ifdef OCR_ENABLE_STATISTICS
        statsWORKER_START(policy, base->guid, base, base->computes[i]->guid, base->computes[i]);
#endif
    }
}

void xeFinishWorker(ocrWorker_t * base) {
    DPRINTF(DEBUG_LVL_INFO, "Finishing worker routine %ld\n", getWorkerId(base));
    ASSERT(base->computeCount == 1); // For now; o/w have to create more launchArg
    u64 i = 0;
    for(i = 0; i < base->computeCount; i++) {
        base->computes[i]->fcts.finish(base->computes[i]);
    }
}

void xeStopWorker(ocrWorker_t * base) {
    ocrWorkerXe_t * xeWorker = (ocrWorkerXe_t *) base;
    xeWorker->run = false;
    
    u64 computeCount = base->computeCount;
    u64 i = 0;
    // This code should be called by the master thread to join others
    for(i = 0; i < computeCount; i++) {
        base->computes[i]->fcts.stop(base->computes[i]);
#ifdef OCR_ENABLE_STATISTICS
        statsWORKER_STOP(base->pd, base->fguid.guid, base->fguid.metaDataPtr,
                         base->computes[i]->fguid.guid,
                         base->computes[i]->fguid.metaDataPtr);
#endif
    }
    DPRINTF(DEBUG_LVL_INFO, "Stopping worker %ld\n", getWorkerId(base));

    // Destroy the GUID
    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);
    
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid) = base->fguid;
    PD_MSG_FIELD(properties) = 0;
    RESULT_ASSERT(base->pd->processMessage(base->pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
    base->fguid.guid = UNINITIALIZED_GUID;
}

bool xeIsRunningWorker(ocrWorker_t * base) {
    ocrWorkerXe_t * xeWorker = (ocrWorkerXe_t *) base;
    return xeWorker->run;
}

u8 xeSendMessage(ocrWorker_t *self, ocrLocation_t location, ocrPolicyMsg_t **msg) {
    ASSERT(self->computeCount == 1);
    return self->computes[0]->fcts.sendMessage(self->computes[0], location, msg);
}

u8 xePollMessage(ocrWorker_t *self, ocrPolicyMsg_t **msg) {
    ASSERT(self->computeCount == 1);
    return self->computes[0]->fcts.pollMessage(self->computes[0], msg);
}

u8 xeWaitMessage(ocrWorker_t *self, ocrLocation_t location, ocrPolicyMsg_t **msg) {
    ASSERT(self->computeCount == 1);
    return self->computes[0]->fcts.waitMessage(self->computes[0], msg);
}

/******************************************************/
/* OCR-XE WORKER FACTORY                              */
/******************************************************/

void destructWorkerFactoryXe(ocrWorkerFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrWorkerFactory_t * newOcrWorkerFactoryXe(ocrParamList_t * perType) {
    ocrWorkerFactoryXe_t* derived = (ocrWorkerFactoryXe_t*)runtimeChunkAlloc(sizeof(ocrWorkerFactoryXe_t), NULL);
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*) derived;
    base->instantiate = newWorkerXe;
    base->destruct =  destructWorkerFactoryXe;
    base->workerFcts.destruct = destructWorkerXe;
    base->workerFcts.start = xeStartWorker;
    base->workerFcts.stop = xeStopWorker;
    base->workerFcts.finish = xeFinishWorker;
    base->workerFcts.isRunning = xeIsRunningWorker;
    base->workerFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrWorker_t*, ocrLocation_t, ocrPolicyMsg_t**), xeSendMessage);
    base->workerFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrWorker_t*, ocrPolicyMsg_t**), xePollMessage);
    base->workerFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrWorker_t*, ocrPolicyMsg_t**), xeWaitMessage);
    return base;
}

#endif /* ENABLE_WORKER_XE */
