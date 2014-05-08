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
#include "ocr-db.h"
#include "worker/xe/xe-worker.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

#define DEBUG_TYPE WORKER

/******************************************************/
/* OCR-XE WORKER                                      */
/******************************************************/

// Convenient to have an id to index workers in pools
static inline u64 getWorkerId(ocrWorker_t * worker) {
    ocrWorkerXe_t * xeWorker = (ocrWorkerXe_t *) worker;
    return xeWorker->id;
}

/**
 * The computation worker routine that asks work to the scheduler
 */
static void workerLoop(ocrWorker_t * worker) {
    ocrPolicyDomain_t *pd = worker->pd;
    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);
    while(worker->fcts.isRunning(worker)) {
        ocrFatGuid_t taskGuid = {.guid = NULL_GUID, .metaDataPtr = NULL};
        u32 count = 1;
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_COMM_TAKE
        msg.type = PD_MSG_COMM_TAKE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD(guids) = &taskGuid;
        PD_MSG_FIELD(guidCount) = count;
        PD_MSG_FIELD(properties) = 0;
        PD_MSG_FIELD(type) = OCR_GUID_EDT;
        if(pd->fcts.processMessage(pd, &msg, true) == 0) {
            // We got a response
            count = PD_MSG_FIELD(guidCount);
            if(count == 1) {
                // REC: TODO: Do we need a message to execute this
                ASSERT(taskGuid.guid != NULL_GUID && taskGuid.metaDataPtr != NULL);
                worker->curTask = (ocrTask_t*)taskGuid.metaDataPtr;
                u8 (*executeFunc)(ocrTask_t *) = (u8 (*)(ocrTask_t*))PD_MSG_FIELD(extra); // Execute is stored in extra
                executeFunc(worker->curTask);
                worker->curTask = NULL;
#undef PD_TYPE
                // Destroy the work
#define PD_TYPE PD_MSG_WORK_DESTROY
                msg.type = PD_MSG_WORK_DESTROY | PD_MSG_REQUEST;
                PD_MSG_FIELD(guid) = taskGuid;
                PD_MSG_FIELD(properties) = 0;
                // Ignore failures, we may be shutting down
                pd->fcts.processMessage(pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
            } else if (count > 1) {
                // TODO: In the future, potentially take more than one)
                ASSERT(0);
            } else {
                // count = 0; no work received; do something else if required.
            }
        }
    } /* End of while loop */
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
    ocrWorker_t * base = (ocrWorker_t*)runtimeChunkAlloc(sizeof(ocrWorkerXe_t), NULL);
    factory->initialize(factory, base, perInstance);
    return base;
}

void initializeWorkerXe(ocrWorkerFactory_t * factory, ocrWorker_t* base, ocrParamList_t * perInstance) {
    initializeWorkerOcr(factory, base, perInstance);
    base->type = SLAVE_WORKERTYPE;

    ocrWorkerXe_t* workerXe = (ocrWorkerXe_t*) base;
    workerXe->id = ((paramListWorkerXeInst_t*)perInstance)->workerId;
    workerXe->running = false;
}

void xeBeginWorker(ocrWorker_t * base, ocrPolicyDomain_t * policy) {

    // Starts everybody, the first comp-platform has specific
    // code to represent the master thread.
    u64 computeCount = base->computeCount;
    ASSERT(computeCount == 1);
    u64 i = 0;
    for(i = 0; i < computeCount; ++i) {
        base->computes[i]->fcts.begin(base->computes[i], policy, base->type);
#ifdef OCR_ENABLE_STATISTICS
        statsWORKER_START(policy, base->guid, base, base->computes[i]->guid, base->computes[i]);
#endif
        base->computes[i]->fcts.setCurrentEnv(base->computes[i], policy, base);
    }
}

void xeStartWorker(ocrWorker_t * base, ocrPolicyDomain_t * policy) {

    // Get a GUID
    guidify(policy, (u64)base, &(base->fguid), OCR_GUID_WORKER);
    base->pd = policy;

    ocrWorkerXe_t * xeWorker = (ocrWorkerXe_t *) base;
    xeWorker->running = true;

    // Starts everybody, the first comp-platform has specific
    // code to represent the master thread.
    u64 computeCount = base->computeCount;
    ASSERT(computeCount == 1);
    u64 i = 0;
    for(i = 0; i < computeCount; i++) {
        base->computes[i]->fcts.start(base->computes[i], policy, base);
#ifdef OCR_ENABLE_STATISTICS
        statsWORKER_START(policy, base->guid, base, base->computes[i]->guid, base->computes[i]);
#endif
    }
}

#ifdef TEMPORARY_FSIM_HACK_TILL_WE_FIGURE_OCR_START_STOP_HANDSHAKES
// FIXME: HACK!!! HACK!!! HACK!!!
// Fwd declaration so we can call the fn directly instead of go
// through a blob function to cook args. Bala needs to resolve this in
// a better way that allows for args in FSIM.
ocrGuid_t mainEdt( u32, u64 *, u32, ocrEdtDep_t * );
#endif

void* xeRunWorker(ocrWorker_t * worker) {
    // Need to pass down a data-structure
    ocrPolicyDomain_t *pd = worker->pd;

    if (pd->myLocation == 0) { //Blessed worker

// FIXME: HACK!!! HACK!!! HACK!!!
// Until Bala resolves how to handle args in FSIM, we will call
// directly into mainEdt. See fwd decl above.
#ifndef TEMPORARY_FSIM_HACK_TILL_WE_FIGURE_OCR_START_STOP_HANDSHAKES
        // This is all part of the mainEdt setup
        // and should be executed by the "blessed" worker.
        void * packedUserArgv = userArgsGet();
        ocrEdt_t mainEdt = mainEdtGet();

        u64 totalLength = ((u64*) packedUserArgv)[0]; // already exclude this first arg
        // strip off the 'totalLength first argument'
        packedUserArgv = (void *) (((u64)packedUserArgv) + sizeof(u64)); // skip first totalLength argument
        ocrGuid_t dbGuid;
        void* dbPtr;
        ocrDbCreate(&dbGuid, &dbPtr, totalLength,
                    DB_PROP_NONE, NULL_GUID, NO_ALLOC);

        // copy packed args to DB
        hal_memCopy(dbPtr, packedUserArgv, totalLength, 0);

        // Create the depv
        ocrEdtDep_t depv;
        depv.guid = dbGuid;
        depv.ptr = dbPtr;

        // Call into mainEdt
        mainEdt(0, NULL, 1, &depv);
#else
        // FIXME: HACK!!! HACK!!! HACK!!
        // Direct call into mainEdt() with NO ARGUMENTS
        mainEdt((u32)0, (u64*)NULL, (u32)0, (ocrEdtDep_t*)NULL);
#endif
    }

    DPRINTF(DEBUG_LVL_INFO, "Starting scheduler routine of worker %ld\n", getWorkerId(worker));
    workerLoop(worker);
    return NULL;
}

void xeFinishWorker(ocrWorker_t * base) {
    DPRINTF(DEBUG_LVL_INFO, "Finishing worker routine %ld\n", getWorkerId(base));
    ASSERT(base->computeCount == 1);
    u64 i = 0;
    for(i = 0; i < base->computeCount; i++) {
        base->computes[i]->fcts.finish(base->computes[i]);
    }
}

void xeStopWorker(ocrWorker_t * base) {
    ocrWorkerXe_t * xeWorker = (ocrWorkerXe_t *) base;
    xeWorker->running = false;

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
    RESULT_ASSERT(base->pd->fcts.processMessage(base->pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
    base->fguid.guid = UNINITIALIZED_GUID;
}

bool xeIsRunningWorker(ocrWorker_t * base) {
    ocrWorkerXe_t * xeWorker = (ocrWorkerXe_t *) base;
    return xeWorker->running;
}

/******************************************************/
/* OCR-XE WORKER FACTORY                              */
/******************************************************/

void destructWorkerFactoryXe(ocrWorkerFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrWorkerFactory_t * newOcrWorkerFactoryXe(ocrParamList_t * perType) {
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*)runtimeChunkAlloc(sizeof(ocrWorkerFactoryXe_t), (void *)1);

    base->instantiate = &newWorkerXe;
    base->initialize = &initializeWorkerXe;
    base->destruct = &destructWorkerFactoryXe;

    base->workerFcts.destruct = FUNC_ADDR(void (*)(ocrWorker_t*), destructWorkerXe);
    base->workerFcts.begin = FUNC_ADDR(void (*)(ocrWorker_t*, ocrPolicyDomain_t*), xeBeginWorker);
    base->workerFcts.start = FUNC_ADDR(void (*)(ocrWorker_t*, ocrPolicyDomain_t*), xeStartWorker);
    base->workerFcts.run = FUNC_ADDR(void* (*)(ocrWorker_t*), xeRunWorker);
    base->workerFcts.stop = FUNC_ADDR(void (*)(ocrWorker_t*), xeStopWorker);
    base->workerFcts.finish = FUNC_ADDR(void (*)(ocrWorker_t*), xeFinishWorker);
    base->workerFcts.isRunning = FUNC_ADDR(bool (*)(ocrWorker_t*), xeIsRunningWorker);
    return base;
}

#endif /* ENABLE_WORKER_XE */
