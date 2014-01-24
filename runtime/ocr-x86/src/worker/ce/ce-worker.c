/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_WORKER_CE

#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime-types.h"
#include "ocr-sysboot.h"
#include "ocr-types.h"
#include "ocr-worker.h"
#include "worker/ce/ce-worker.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

#define DEBUG_TYPE WORKER

/******************************************************/
/* OCR-CE WORKER                                      */
/******************************************************/

/**
 * The computation worker routine that asks work to the scheduler
 */
static void * workerRoutine(launchArg_t * arg);

// Convenient to have an id to index workers in pools
static inline u64 getWorkerId(ocrWorker_t * worker) {
    ocrWorkerCe_t * ceWorker = (ocrWorkerCe_t *) worker;
    return ceWorker->id;
}

static void workerLoop(ocrWorker_t * worker) {
    ocrPolicyDomain_t *pd = worker->pd;
    ocrPolicyMsg_t *msgPtr;
    while(worker->fcts.isRunning(worker)) {
        worker->fcts.waitMessage(worker, &msgPtr);
        if(pd->processMessage(pd, msgPtr, true) == 0) {
            pd->pdFree(pd, msgPtr);
        } else {
            ASSERT(0); // we don't know (yet) what to do if something fails!
        }
    } /* End of while loop */
}

static void * workerRoutine(launchArg_t* launchArg) {
    // Need to pass down a data-structure
    ocrWorker_t * worker = (ocrWorker_t *) launchArg->arg;
    ocrPolicyDomain_t *pd = worker->pd;
    pd->pdFree(pd, launchArg); // Free the launch argument

    // Set who we are
    u32 i;
    for(i = 0; i < worker->computeCount; ++i) {
        worker->computes[i]->fcts.setCurrentEnv(worker->computes[i], pd, worker);
    }
    
    DPRINTF(DEBUG_LVL_INFO, "Starting scheduler routine of CE worker %ld\n", getWorkerId(worker));
    workerLoop(worker);
    return NULL;
}

void destructWorkerCe(ocrWorker_t * base) {
    u64 i = 0;
    while(i < base->computeCount) {
        base->computes[i]->fcts.destruct(base->computes[i]);
        ++i;
    }
    runtimeChunkFree((u64)(base->computes), NULL);    
    runtimeChunkFree((u64)base, NULL);
}

/**
 * Builds an instance of a CE worker
 */
ocrWorker_t* newWorkerCe (ocrWorkerFactory_t * factory,
                          ocrParamList_t * perInstance) {
    ocrWorkerCe_t * worker = (ocrWorkerCe_t*)runtimeChunkAlloc(
        sizeof(ocrWorkerCe_t), NULL);
    ocrWorker_t * base = (ocrWorker_t *) worker;

    base->fguid.guid = UNINITIALIZED_GUID;
    base->fguid.metaDataPtr = base;
    base->pd = NULL;
    base->curTask = NULL;
    base->fcts = factory->workerFcts;
    base->type = ((paramListWorkerCeInst_t*)perInstance)->workerType;
    ASSERT(base->type == MASTER_WORKERTYPE);
    
    worker->id = ((paramListWorkerCeInst_t*)perInstance)->workerId;
    worker->run = false;

    base->fcts = factory->workerFcts;
    return base;
}

void ceBeginWorker(ocrWorker_t * base, ocrPolicyDomain_t * policy) {
    
    // Starts everybody, the first comp-platform has specific
    // code to represent the master thread.
    u64 computeCount = base->computeCount;
    ASSERT(computeCount == 1); // For now; o/w have to create more launchArg
    u64 i = 0;
    for(i = 0; i < computeCount; ++i) {
        base->computes[i]->fcts.begin(base->computes[i], policy, base->type);
#ifdef OCR_ENABLE_STATISTICS
        statsWORKER_START(policy, base->guid, base, base->computes[i]->guid, base->computes[i]);
#endif
    }

    if(base->type == MASTER_WORKERTYPE) {
        // For the master thread, we need to set the PD and worker
        // The other threads will set this when they start
        for(i = 0; i < computeCount; ++i) {
            base->computes[i]->fcts.setCurrentEnv(base->computes[i], policy, base);
        }
    }
}

void ceStartWorker(ocrWorker_t * base, ocrPolicyDomain_t * policy) {

    ocrWorkerCe_t * ceWorker = (ocrWorkerCe_t *) base;
    if(base->type == MASTER_WORKERTYPE) {
        if(!ceWorker->secondStart) {
            ceWorker->secondStart = true;
            return; // Don't start right away
        }
    }
    
    // Get a GUID
    guidify(policy, (u64)base, &(base->fguid), OCR_GUID_WORKER);
    base->pd = policy;

    ASSERT(base->type != MASTER_WORKERTYPE || ceWorker->secondStart);
    ceWorker->run = true;

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
        base->computes[i]->fcts.start(base->computes[i], policy, SINGLE_WORKERTYPE, launchArg);
#ifdef OCR_ENABLE_STATISTICS
        statsWORKER_START(policy, base->guid, base, base->computes[i]->guid, base->computes[i]);
#endif
    }
}

void ceFinishWorker(ocrWorker_t * base) {
    DPRINTF(DEBUG_LVL_INFO, "Finishing worker routine %ld\n", getWorkerId(base));
    ASSERT(base->computeCount == 1); // For now; o/w have to create more launchArg
    u64 i = 0;
    for(i = 0; i < base->computeCount; i++) {
        base->computes[i]->fcts.finish(base->computes[i]);
    }
}

void ceStopWorker(ocrWorker_t * base) {
    ocrWorkerCe_t * ceWorker = (ocrWorkerCe_t *) base;
    ceWorker->run = false;
    
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

bool ceIsRunningWorker(ocrWorker_t * base) {
    ocrWorkerCe_t * ceWorker = (ocrWorkerCe_t *) base;
    return ceWorker->run;
}

u8 ceSendMessage(ocrWorker_t *self, ocrLocation_t location, ocrPolicyMsg_t **msg) {
    ASSERT(self->computeCount == 1);
    return self->computes[0]->fcts.sendMessage(self->computes[0], location, msg);
}

u8 cePollMessage(ocrWorker_t *self, ocrPolicyMsg_t **msg, u32 mask) {
    ASSERT(self->computeCount == 1);
    return self->computes[0]->fcts.pollMessage(self->computes[0], msg, mask);
}

u8 ceWaitMessage(ocrWorker_t *self, ocrLocation_t location, ocrPolicyMsg_t **msg) {
    ASSERT(self->computeCount == 1);
    return self->computes[0]->fcts.waitMessage(self->computes[0], msg);
}

/******************************************************/
/* OCR-CE WORKER FACTORY                              */
/******************************************************/

void destructWorkerFactoryCe(ocrWorkerFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrWorkerFactory_t * newOcrWorkerFactoryCe(ocrParamList_t * perType) {
    ocrWorkerFactoryCe_t* derived = (ocrWorkerFactoryCe_t*)runtimeChunkAlloc(sizeof(ocrWorkerFactoryCe_t), (void *)1);
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*) derived;
    base->instantiate = newWorkerCe;
    base->destruct =  destructWorkerFactoryCe;
    base->workerFcts.destruct = FUNC_ADDR(void (*)(ocrWorker_t*), destructWorkerCe);
    base->workerFcts.begin = FUNC_ADDR(void (*)(ocrWorker_t*, ocrPolicyDomain_t*), ceBeginWorker);
    base->workerFcts.start = FUNC_ADDR(void (*)(ocrWorker_t*, ocrPolicyDomain_t*), ceStartWorker);
    base->workerFcts.stop = FUNC_ADDR(void (*)(ocrWorker_t*), ceStopWorker);
    base->workerFcts.finish = FUNC_ADDR(void (*)(ocrWorker_t*), ceFinishWorker);
    base->workerFcts.isRunning = FUNC_ADDR(bool (*)(ocrWorker_t*), ceIsRunningWorker);
    base->workerFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrWorker_t*, ocrLocation_t, ocrPolicyMsg_t**), ceSendMessage);
    base->workerFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrWorker_t*, ocrPolicyMsg_t**, u32), cePollMessage);
    base->workerFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrWorker_t*, ocrPolicyMsg_t**), ceWaitMessage);
    return base;
}

#endif /* ENABLE_WORKER_CE */
