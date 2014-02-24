/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_WORKER_HC

#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-db.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime-types.h"
#include "ocr-sysboot.h"
#include "ocr-types.h"
#include "ocr-worker.h"
#include "worker/hc/hc-worker.h"

#include "experimental/ocr-placer.h"
#include "extensions/ocr-affinity.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

#ifdef OCR_RUNTIME_PROFILER
#include "utils/profiler/profiler.h"
#endif

#define DEBUG_TYPE WORKER

/******************************************************/
/* OCR-HC WORKER                                      */
/******************************************************/

// Convenient to have an id to index workers in pools
static inline u64 getWorkerId(ocrWorker_t * worker) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) worker;
    return hcWorker->id;
}

static void hcWorkShift(ocrWorker_t * worker) {
    ocrPolicyDomain_t * pd;
    PD_MSG_STACK(msg);
    getCurrentEnv(&pd, NULL, NULL, &msg);
    ocrFatGuid_t taskGuid = {.guid = NULL_GUID, .metaDataPtr = NULL};
    u32 count = 1;
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_COMM_TAKE
    msg.type = PD_MSG_COMM_TAKE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guids) = &taskGuid;
    PD_MSG_FIELD_IO(guidCount) = count;
    PD_MSG_FIELD_I(properties) = 0;
    PD_MSG_FIELD_IO(type) = OCR_GUID_EDT;
    // TODO: In the future, potentially take more than one)
    if(pd->fcts.processMessage(pd, &msg, true) == 0) {
        // We got a response
        count = PD_MSG_FIELD_IO(guidCount);
        if(count == 1) {
            ASSERT(taskGuid.guid != NULL_GUID && taskGuid.metaDataPtr != NULL);
            worker->curTask = (ocrTask_t*)taskGuid.metaDataPtr;
            DPRINTF(DEBUG_LVL_VERB, "Worker shifting to execute EDT GUID 0x%lx\n", taskGuid.guid);
            u8 (*executeFunc)(ocrTask_t *) = (u8 (*)(ocrTask_t*))PD_MSG_FIELD_IO(extra); // Execute is stored in extra
            executeFunc(worker->curTask);
            // Mark the task. Allows the PD to check the state of workers
            // and detect quiescence, without weird behavior because curTask
            // is being deallocated in parallel
            //TODO this is going away with the fix for runlevel #80
            worker->curTask = (ocrTask_t*) 0x1;
            // Destroy the work
#undef PD_TYPE

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_WORK_DESTROY
            getCurrentEnv(NULL, NULL, NULL, &msg);
            msg.type = PD_MSG_WORK_DESTROY | PD_MSG_REQUEST;
            PD_MSG_FIELD_I(guid) = taskGuid;
            PD_MSG_FIELD_I(currentEdt) = taskGuid;
            PD_MSG_FIELD_I(properties) = 0;
            // Ignore failures, we may be shutting down
            pd->fcts.processMessage(pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
            // Important for this to be the last
            worker->curTask = NULL;
        }
    }
}

/**
 * The computation worker routine that asks work to the scheduler
 */
static void workerLoop(ocrWorker_t * worker) {
    while(worker->fcts.isRunning(worker)) {
        START_PROFILE(wo_hc_workerLoop);
        worker->fcts.workShift(worker);
        EXIT_PROFILE;
    } /* End of while loop */
}

void destructWorkerHc(ocrWorker_t * base) {
    u64 i = 0;
    while(i < base->computeCount) {
        base->computes[i]->fcts.destruct(base->computes[i]);
        ++i;
    }
    runtimeChunkFree((u64)(base->computes), NULL);
    runtimeChunkFree((u64)base, NULL);
}

void hcBeginWorker(ocrWorker_t * base, ocrPolicyDomain_t * policy) {

    // Starts everybody, the first comp-platform has specific
    // code to represent the master thread.
    u64 computeCount = base->computeCount;
    base->location = (ocrLocation_t)base;
    ASSERT(computeCount == 1);
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

void hcStartWorker(ocrWorker_t * base, ocrPolicyDomain_t * policy) {

    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;

    if(!hcWorker->secondStart) {
        // Get a GUID
        guidify(policy, (u64)base, &(base->fguid), OCR_GUID_WORKER);
        base->pd = policy;
        hcWorker->running = true;
        if(base->type == MASTER_WORKERTYPE) {
            hcWorker->secondStart = true; // Only relevant for MASTER_WORKERTYPE
            return; // Don't start right away
        }
    }

    ASSERT(base->type != MASTER_WORKERTYPE || hcWorker->secondStart);

    // Starts everybody, the first comp-platform has specific
    // code to represent the master thread.
    u64 computeCount = base->computeCount;
    // What the compute target will execute
    ASSERT(computeCount == 1);
    u64 i = 0;
    for(i = 0; i < computeCount; ++i) {
        base->computes[i]->fcts.start(base->computes[i], policy, base);
#ifdef OCR_ENABLE_STATISTICS
        statsWORKER_START(policy, base->guid, base, base->computes[i]->guid, base->computes[i]);
#endif
    }
    // Otherwise, it is highly likely that we are shutting down
}

static bool isMainEdtForker(ocrWorker_t * worker, ocrGuid_t * affinityMasterPD) {
    // Determine if current worker is the master worker of this PD
    bool blessedWorker = (worker->type == MASTER_WORKERTYPE);
    // When OCR is used in library mode, there's no mainEdt
    blessedWorker &= (mainEdtGet() != NULL);
    if (blessedWorker) {
        // Determine if current master worker is part of master PD
        u64 count = 0;
        // There should be a single master PD
        ASSERT(!ocrAffinityCount(AFFINITY_PD_MASTER, &count) && (count == 1));
        ocrAffinityGet(AFFINITY_PD_MASTER, &count, affinityMasterPD);
        ASSERT(count == 1);
        blessedWorker &= (worker->pd->myLocation == affinityToLocation(*affinityMasterPD));
    }
    return blessedWorker;
}

void* hcRunWorker(ocrWorker_t * worker) {
    ocrGuid_t affinityMasterPD;
    bool forkMain = isMainEdtForker(worker, &affinityMasterPD);
    if (forkMain) {
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
                    DB_PROP_IGNORE_WARN, affinityMasterPD, NO_ALLOC);

        // copy packed args to DB
        hal_memCopy(dbPtr, packedUserArgv, totalLength, 0);

        // Prepare the mainEdt for scheduling
        ocrGuid_t edtTemplateGuid = NULL_GUID, edtGuid = NULL_GUID;
        ocrEdtTemplateCreate(&edtTemplateGuid, mainEdt, 0, 1);
        ocrEdtCreate(&edtGuid, edtTemplateGuid, EDT_PARAM_DEF, /* paramv = */ NULL,
                     /* depc = */ EDT_PARAM_DEF, /* depv = */ &dbGuid,
                     EDT_PROP_NONE, affinityMasterPD, NULL);
    } else {
        // Set who we are
        ocrPolicyDomain_t *pd = worker->pd;
        u32 i;
        for(i = 0; i < worker->computeCount; ++i) {
            worker->computes[i]->fcts.setCurrentEnv(worker->computes[i], pd, worker);
        }
    }

    DPRINTF(DEBUG_LVL_INFO, "Starting scheduler routine of worker %ld\n", getWorkerId(worker));
    workerLoop(worker);
    return NULL;
}

void hcFinishWorker(ocrWorker_t * base) {
    DPRINTF(DEBUG_LVL_INFO, "Finishing worker routine %ld\n", getWorkerId(base));
    ASSERT(base->computeCount == 1);
    u64 i = 0;
    for(i = 0; i < base->computeCount; i++) {
        base->computes[i]->fcts.finish(base->computes[i]);
    }
}

void hcStopWorker(ocrWorker_t * base) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    // Makes worker threads to exit their routine.
    hcWorker->running = false;
    ASSERT(base->pd != NULL);
    u64 computeCount = base->computeCount;
    u64 i = 0;
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
    PD_MSG_STACK(msg);
    getCurrentEnv(NULL, NULL, NULL, &msg);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD_I(guid) = base->fguid;
    PD_MSG_FIELD_I(properties) = 0;
    ASSERT(base->pd != NULL);
    // Ignore failure here, we are most likely shutting down
    base->pd->fcts.processMessage(base->pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
    base->fguid.guid = UNINITIALIZED_GUID;
}

bool hcIsRunningWorker(ocrWorker_t * base) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    return hcWorker->running;
}

/**
 * @brief Builds an instance of a HC worker
 */
ocrWorker_t* newWorkerHc(ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    ocrWorker_t * worker = (ocrWorker_t*) runtimeChunkAlloc( sizeof(ocrWorkerHc_t), PERSISTENT_CHUNK);
    factory->initialize(factory, worker, perInstance);
    return worker;
}

/**
 * @brief Initialize an instance of a HC worker
 */
void initializeWorkerHc(ocrWorkerFactory_t * factory, ocrWorker_t* self, ocrParamList_t * perInstance) {
    initializeWorkerOcr(factory, self, perInstance);
    self->type = ((paramListWorkerHcInst_t*)perInstance)->workerType;
    u64 workerId = ((paramListWorkerHcInst_t*)perInstance)->workerId;;
    ASSERT((workerId && self->type == SLAVE_WORKERTYPE) ||
           (workerId == 0 && self->type == MASTER_WORKERTYPE));

    ocrWorkerHc_t * workerHc = (ocrWorkerHc_t*) self;
    workerHc->id = workerId;
    workerHc->running = false;
    workerHc->secondStart = false;
    workerHc->hcType = HC_WORKER_COMP;
}

/******************************************************/
/* OCR-HC WORKER FACTORY                              */
/******************************************************/

void destructWorkerFactoryHc(ocrWorkerFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrWorkerFactory_t * newOcrWorkerFactoryHc(ocrParamList_t * perType) {
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*)runtimeChunkAlloc(sizeof(ocrWorkerFactoryHc_t), NONPERSISTENT_CHUNK);

    base->instantiate = &newWorkerHc;
    base->initialize = &initializeWorkerHc;
    base->destruct = &destructWorkerFactoryHc;

    base->workerFcts.destruct = FUNC_ADDR(void (*) (ocrWorker_t *), destructWorkerHc);
    base->workerFcts.begin = FUNC_ADDR(void (*) (ocrWorker_t *, ocrPolicyDomain_t *), hcBeginWorker);
    base->workerFcts.start = FUNC_ADDR(void (*) (ocrWorker_t *, ocrPolicyDomain_t *), hcStartWorker);
    base->workerFcts.run = FUNC_ADDR(void* (*) (ocrWorker_t *), hcRunWorker);
    base->workerFcts.workShift = FUNC_ADDR(void* (*) (ocrWorker_t *), hcWorkShift);
    base->workerFcts.stop = FUNC_ADDR(void (*) (ocrWorker_t *), hcStopWorker);
    base->workerFcts.finish = FUNC_ADDR(void (*) (ocrWorker_t *), hcFinishWorker);
    base->workerFcts.isRunning = FUNC_ADDR(bool (*) (ocrWorker_t *), hcIsRunningWorker);
    return base;
}

#endif /* ENABLE_WORKER_HC */
